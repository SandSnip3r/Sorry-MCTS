#ifndef SORRY_MCTS_HPP_
#define SORRY_MCTS_HPP_

#include "action.hpp"

#include <chrono>
#include <random>

class Node;
class LoopCondition;

namespace sorry {
class Sorry;
} // namespace sorry

class SorryMcts {
public:
  SorryMcts(double explorationConstant);
  sorry::Action pickBestAction(const sorry::Sorry &startingState, int rolloutCount);
  sorry::Action pickBestAction(const sorry::Sorry &startingState, std::chrono::duration<double> timeLimit);
private:
  const double explorationConstant_;
  std::mt19937 eng_{0};
  sorry::Action pickBestAction(const sorry::Sorry &startingState, LoopCondition *loopCondition);
  void doSingleStep(const sorry::Sorry &startingState, Node *rootNode);
  int select(const Node *currentNode, bool withExploration, const std::vector<size_t> &indices);
  int rollout(sorry::Sorry state);
  void backprop(Node *current, int moveCount);
  double nodeScore(const Node *current, const Node *parent, double maxAverageMoveCount, double minAverageMoveCount, bool withExploration) const;
  void printActions(const Node *current, int levels, int currentLevel=0) const;
};

#endif // SORRY_MCTS_HPP_