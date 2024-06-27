#include "common.hpp"
#include "sorry.hpp"
#include "sorryMcts.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <numeric>
#include <thread>

class TimeLoopCondition : public internal::LoopCondition {
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

class CountCondition : public internal::LoopCondition {
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

void ExplicitTerminator::setDone(bool done) {
  done_ = done;
}

bool ExplicitTerminator::condition() const {
  return !done_;
}

void ExplicitTerminator::oneIterationComplete() {}

using namespace sorry;

struct Node {
  explicit Node(const Sorry &s) : state(s) {}
  Node(const Sorry &s, const Action &a, Node *p) : state(s), action(a), parent(p) {}
  ~Node() {
    for (Node *successor : successors) {
      delete successor;
    }
  }
  Sorry state;
  Action action;
  Node *parent{nullptr};
  std::vector<Node*> successors;
  std::array<int, 4> winCount = {0,0,0,0};
  int gameCount{0}; // TODO: Maybe can be removed

  double score() const {
    if (gameCount == 0) {
      throw std::runtime_error("Cannot get score of node with no games");
    }
    return winCount.at(static_cast<int>(action.playerColor)) / static_cast<double>(gameCount);
  }
};

SorryMcts::SorryMcts(double explorationConstant) : explorationConstant_(explorationConstant) {
  eng_ = createRandomEngine();
}

void SorryMcts::run(const Sorry &startingState, int rolloutCount) {
  CountCondition condition(rolloutCount);
  run(startingState, &condition);
}

void SorryMcts::run(const Sorry &startingState, std::chrono::duration<double> timeLimit) {
  TimeLoopCondition condition(timeLimit);
  run(startingState, &condition);
}

void SorryMcts::run(const Sorry &startingState, internal::LoopCondition *loopCondition) {
  // Since we've been invoked, we know that we are the current player.
  ourPlayer_ = startingState.getPlayerTurn();
  {
    std::unique_lock lock(treeMutex_);
    if (rootNode_ != nullptr) {
      delete rootNode_;
    }
    rootNode_ = new Node(startingState);
    iterationCount_ = 0;
  }
  if (startingState.getActions().size() == 0) {
    // No actions, must be done with the game.
    return;
  }
  while (loopCondition->condition()) {
    doSingleStep(startingState);
    ++iterationCount_;
    if (startingState.getActions().size() == 1) {
      // If there's only one option, we're done.
      return;
    }
    loopCondition->oneIterationComplete();
  }
}

void SorryMcts::reset() {
  std::unique_lock lock(treeMutex_);
  if (rootNode_ != nullptr) {
    delete rootNode_;
    rootNode_ = nullptr;
  }
}

sorry::Action SorryMcts::pickBestAction() const {
  if (rootNode_ == nullptr) {
    throw std::runtime_error("Asking for best action, but have no root node");
  }
  // TODO: The below code assumes that all possible actions have been visited once.
  std::vector<size_t> indices(rootNode_->successors.size());
  std::iota(indices.begin(), indices.end(), 0);
  int index = select(rootNode_, /*withExploration=*/false, indices);
  // printActions(rootNode_, 2);
  return rootNode_->successors.at(index)->action;
}

std::vector<ActionScore> SorryMcts::getActionScores() const {
  std::unique_lock lock(treeMutex_);
  if (rootNode_ == nullptr) {
    // No known actions yet.
    return {};
  }
  std::vector<size_t> indices(rootNode_->successors.size());
  std::iota(indices.begin(), indices.end(), 0);
  std::vector<ActionScore> result;
  for (size_t index : indices) {
    const Node *successor = rootNode_->successors.at(index);
    const double score = nodeScore(successor, rootNode_, /*withExploration=*/false);
    result.emplace_back(ActionScore{.action=successor->action,
                                    .score=score});
  }
  return result;
}

std::vector<double> SorryMcts::getWinRates() const {
  std::unique_lock lock(treeMutex_);
  if (rootNode_ == nullptr) {
    return { 0.25, 0.25, 0.25, 0.25 };
  }
  if (rootNode_->successors.empty()) {
    return { 0.25, 0.25, 0.25, 0.25 };
  }
  std::vector<size_t> indices(rootNode_->successors.size());
  std::iota(indices.begin(), indices.end(), 0);
  const int indexOfPreferredAction = select(rootNode_, false, indices);
  Node *preferredAction = rootNode_->successors.at(indexOfPreferredAction);
  const double sum = preferredAction->winCount[0] + preferredAction->winCount[1] + preferredAction->winCount[2] + preferredAction->winCount[3];
  if (sum == 0) {
    return { 0.25, 0.25, 0.25, 0.25 };
  }
  return { preferredAction->winCount[0] / sum,
           preferredAction->winCount[1] / sum,
           preferredAction->winCount[2] / sum,
           preferredAction->winCount[3] / sum };
};


int SorryMcts::getIterationCount() const {
  std::unique_lock lock(treeMutex_);
  return iterationCount_;
}

void getTreeLevelSizes(Node *node, std::vector<int> &sizes, int level=0) {
  if (sizes.size() < static_cast<size_t>(level+1)) {
    sizes.push_back(0);
  }
  ++sizes[level];
  for (Node *child : node->successors) {
    getTreeLevelSizes(child, sizes, level+1);
  }
}

void SorryMcts::doSingleStep(const Sorry &startingState) {
  constexpr int kDepthOfConcreteHands{1};
  Sorry state = startingState;
  std::unique_lock lock(treeMutex_);
  Node *currentNode = rootNode_;
  // Depth represents how many times a player's turn has changed.
  int depth=0;
  while (!state.gameDone()) {
    if (depth < kDepthOfConcreteHands) {
      state.giveOpponentsRandomHands(eng_);
    }
    // Get all actions.
    const auto actions = state.getActions();
    const sorry::PlayerColor currentPlayer = state.getPlayerTurn();
    std::vector<size_t> indices;
    for (const Action &action : actions) {
      // If we don't yet have a node for this action, select it.
      bool foundOurAction = false;
      for (size_t i=0; i<currentNode->successors.size(); ++i) {
        if (currentNode->successors.at(i)->state.equalForPlayer(state, ourPlayer_) &&
            currentNode->successors.at(i)->action == action) {
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
      // std::cout << "New node. Action: " << action.toString() << ", state: " << state.toString() << std::endl;
      // Never tried this action. Create a node for it and then rollout.
      currentNode->successors.push_back(new Node(state, action, currentNode));
      state.doAction(action, eng_);

      // Unlock the mutex protecting the root node during rollout.
      lock.unlock();
      const sorry::PlayerColor winner = rollout(state);
      lock.lock();

      // Propagate the result of the rollout back up through the parents.
      backprop(currentNode->successors.back(), winner);
      // Step is done
      // static int numTimes{1};
      // if (numTimes % 100 == 0) {
      //   std::vector<int> sizes;
      //   getTreeLevelSizes(rootNode_, sizes);
      //   std::cout << "Sizes: ";
      //   for (int i : sizes) {
      //     std::cout << i << ", ";
      //   }
      //   std::cout << std::endl;
      // }
      // ++numTimes;
      return;
    }
    // All possible actions have been seen before. Select one.
    int index = select(currentNode, /*withExploration=*/true, indices);
    currentNode = currentNode->successors.at(index);
    state.doAction(currentNode->action, eng_);
    if (state.getPlayerTurn() != currentPlayer) {
      ++depth;
    }
  }
  // Game is done.
  backprop(currentNode, state.getWinner());
}

int SorryMcts::select(const Node *currentNode, bool withExploration, const std::vector<size_t> &indices) const {
  if (indices.size() == 1) {
    return indices.at(0);
  }
  std::vector<double> scores;
  for (size_t index : indices) {
    const Node *successor = currentNode->successors.at(index);
    const double score = nodeScore(successor, currentNode, withExploration);
    scores.push_back(score);
  }
  auto it = std::max_element(scores.begin(), scores.end());
  return indices.at(distance(scores.begin(), it));
}

sorry::PlayerColor SorryMcts::rollout(Sorry state) {
  while (!state.gameDone()) {
    const auto actions = state.getActions();
    if (actions.empty()) {
      throw std::runtime_error("No actions to take");
    }
    std::uniform_int_distribution<int> dist(0, actions.size()-1);
    const auto &action = actions.at(dist(eng_));
    state.doAction(action, eng_);
  }
  // Game is over.
  return state.getWinner();
}

void SorryMcts::backprop(Node *current, sorry::PlayerColor winner) {
  while (1) {
    ++current->winCount[static_cast<int>(winner)];
    ++current->gameCount;
    if (current->parent == nullptr) {
      // Reached the root. We're done.
      break;
    }
    current = current->parent;
  }
}

double SorryMcts::nodeScore(const Node *current, const Node *parent, bool withExploration) const {
  if (current->gameCount == 0) {
    return 0;
  }
  const double score = current->score();
  if (!withExploration) {
    return score;
  }
  return score + explorationConstant_ * sqrt(log(parent->gameCount) / current->gameCount);
}

void SorryMcts::printActions(const Node *current, int levels, int currentLevel) const {
  // if (currentLevel == levels) {
  //   return;
  // }
  // std::vector<size_t> indices(current->successors.size());
  // std::iota(indices.begin(), indices.end(), 0);
  // for (const Node *successor : current->successors) {
  //   const double score = nodeScore(successor, current, /*withExploration=*/false);
  //   printf("%s[%7.5f] Action %27s average %5.2f moves, count: %5d, parent count: %6d\n", std::string(currentLevel*2, ' ').c_str(), score, successor->action.toString().c_str(), successor->averageMoveCount(), successor->gameCount, current->gameCount);
  //   printActions(successor, levels, currentLevel+1);
  // }
}