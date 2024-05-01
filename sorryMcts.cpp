#include "common.hpp"
#include "sorry.hpp"
#include "sorryMcts.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <numeric>

class LoopCondition {
public:
  virtual bool condition() const = 0;
  virtual void oneIterationComplete() = 0;
};

class TimeLoopCondition : public LoopCondition {
public:
  TimeLoopCondition(std::chrono::duration<double> timeLimit) : startTime_(std::chrono::high_resolution_clock::now()), timeLimit_(timeLimit) {}
  bool condition() const override {
    return std::chrono::high_resolution_clock::now() < startTime_+timeLimit_;
  }
  void oneIterationComplete() override {}
private:
  const std::chrono::high_resolution_clock::time_point startTime_;
  const std::chrono::duration<double> timeLimit_;
};

class CountCondition : public LoopCondition {
public:
  CountCondition(int count) : count_(count) {}
  bool condition() const override {
    return current_ < count_;
  }
  void oneIterationComplete() override {
    ++current_;
  }
private:
  const int count_;
  int current_{0};
};

struct Node {
  Node() = default;
  Node(const Action &a, Node *p) : action(a), parent(p) {}
  ~Node() {
    for (Node *successor : successors) {
      delete successor;
    }
  }
  Action action;
  Node *parent{nullptr};
  std::vector<Node*> successors;
  int totalMoveCount{0};
  int gameCount{0};

  double averageMoveCount() const {
    if (gameCount == 0) {
      throw std::runtime_error("Cannot get average with 0 games");
    }
    return totalMoveCount / static_cast<double>(gameCount);
  }

  std::pair<double, double> getMinAndMaxAverageMoveCountOfSuccessors(const std::vector<size_t> &indices) const {
    double minAverageMoveCount = std::numeric_limits<double>::max();
    double maxAverageMoveCount = 0;
    for (size_t index : indices) {
      const Node *successor = successors.at(index);
      const double average = successor->averageMoveCount();
      if (average < minAverageMoveCount) {
        minAverageMoveCount = average;
      }
      if (average > maxAverageMoveCount) {
        maxAverageMoveCount = average;
      }
    }
    return {minAverageMoveCount, maxAverageMoveCount};
  }
};

SorryMcts::SorryMcts(double explorationConstant) : explorationConstant_(explorationConstant) {
  eng_ = createRandomEngine();
}

Action SorryMcts::pickBestAction(const Sorry &startingState, int rolloutCount) {
  CountCondition condition(rolloutCount);
  return pickBestAction(startingState, &condition);
}

Action SorryMcts::pickBestAction(const Sorry &startingState, std::chrono::duration<double> timeLimit) {
  TimeLoopCondition condition(timeLimit);
  return pickBestAction(startingState, &condition);
}

Action SorryMcts::pickBestAction(const Sorry &startingState, LoopCondition *loopCondition) {
  {
    // Do a quick check and sidestep the whole process if there's only one possible action.
    const auto& actions = startingState.getActions();
    if (actions.size() == 1) {
      return actions.at(0);
    }
  }
  Node rootNode;
  while (loopCondition->condition()) {
    doSingleStep(startingState, &rootNode);
    loopCondition->oneIterationComplete();
  }
  // TODO: The below code assumes that all possible actions have been visited once.
  std::vector<size_t> indices(rootNode.successors.size());
  std::iota(indices.begin(), indices.end(), 0);
  int index = select(&rootNode, /*withExploration=*/false, indices);
  // printActions(&rootNode, 2);
  return rootNode.successors.at(index)->action;
}

void SorryMcts::doSingleStep(const Sorry &startingState, Node *rootNode) {
  Sorry state = startingState;
  Node *currentNode = rootNode;
  while (!state.gameDone()) {
    // Get all actions.
    const auto actions = state.getActions();
    bool rolledOut=false;
    std::vector<size_t> indices;
    for (const Action &action : actions) {
      // If we don't yet have a node for this action, select it.
      bool foundOurAction = false;
      for (size_t i=0; i<currentNode->successors.size(); ++i) {
        if (currentNode->successors.at(i)->action == action) {
          // This is our action.
          indices.push_back(i);
          foundOurAction = true;
          break;
        }
      }
      if (foundOurAction) {
        // Already have a child for this action.
        continue;
      }
      // Never visited this node, expand to it then rollout.
      currentNode->successors.push_back(new Node(action, currentNode));
      state.doAction(action, eng_);
      int result = rollout(state);
      // Propagate the result of the rollout back up through the parents.
      backprop(currentNode->successors.back(), result);
      rolledOut = true;
      break;
    }
    if (rolledOut) {
      return;
    }
    // All possible actions have been seen before. Select one.
    int index = select(currentNode, /*withExploration=*/true, indices);
    currentNode = currentNode->successors.at(index);
    state.doAction(currentNode->action, eng_);
  }
  backprop(currentNode, state.getTotalActionCount());
}

int SorryMcts::select(const Node *currentNode, bool withExploration, const std::vector<size_t> &indices) {
  if (indices.size() == 1) {
    return indices.at(0);
  }
  std::vector<double> scores;
  const auto [minAverageMoveCount, maxAverageMoveCount] = currentNode->getMinAndMaxAverageMoveCountOfSuccessors(indices);
  for (size_t index : indices) {
    const Node *successor = currentNode->successors.at(index);
    const double score = nodeScore(successor, currentNode, maxAverageMoveCount, minAverageMoveCount, withExploration);
    scores.push_back(score);
  }
  auto it = std::max_element(scores.begin(), scores.end());
  return indices.at(distance(scores.begin(), it));
}

int SorryMcts::rollout(Sorry state) {
  int count=0;
  while (!state.gameDone()) {
    const auto actions = state.getActions();
    if (actions.empty()) {
      throw std::runtime_error("No actions to take");
    }
    std::uniform_int_distribution<int> dist(0, actions.size()-1);
    const auto action = actions[dist(eng_)];
    state.doAction(action, eng_);
    ++count;
  }
  // Game is over, return the number of actions taken to reach this point.
  return state.getTotalActionCount();
}

void SorryMcts::backprop(Node *current, int moveCount) {
  while (1) {
    current->totalMoveCount += moveCount;
    ++current->gameCount;
    if (current->parent == nullptr) {
      break;
    }
    current = current->parent;
  }
}

double SorryMcts::nodeScore(const Node *current, const Node *parent, double maxAverageMoveCount, double minAverageMoveCount, bool withExploration) const {
  double score = 1 - (current->averageMoveCount() - minAverageMoveCount) / (maxAverageMoveCount - minAverageMoveCount);
  // double score = 1 - current->averageMoveCount() / maxAverageMoveCount; // Non-scaled version
  if (!withExploration) {
    return score;
  }
  return score + explorationConstant_ * sqrt(log(parent->gameCount) / current->gameCount);
}

void SorryMcts::printActions(const Node *current, int levels, int currentLevel) const {
  if (currentLevel == levels) {
    return;
  }
  std::vector<size_t> indices(current->successors.size());
  std::iota(indices.begin(), indices.end(), 0);
  const auto [minAverageMoveCount, maxAverageMoveCount] = current->getMinAndMaxAverageMoveCountOfSuccessors(indices);
  for (const Node *successor : current->successors) {
    const double score = nodeScore(successor, current, maxAverageMoveCount, minAverageMoveCount, /*withExploration=*/false);
    printf("%s[%7.5f] Action %22s average %5.2f moves, count: %5d, parent count: %6d\n", std::string(currentLevel*2, ' ').c_str(), score, successor->action.toString().c_str(), successor->averageMoveCount(), successor->gameCount, current->gameCount);
    printActions(successor, levels, currentLevel+1);
  }
}