#ifndef SORRY_MCTS_HPP_
#define SORRY_MCTS_HPP_

#include "action.hpp"

#include <chrono>
#include <random>

class Node;
class Sorry;
class LoopCondition;

class SorryMcts {
public:
  SorryMcts(double explorationConstant);
  Action pickBestAction(const Sorry &startingState, int rolloutCount);
  Action pickBestAction(const Sorry &startingState, std::chrono::duration<double> timeLimit);
private:
  const double explorationConstant_;
  std::mt19937 eng_{0};
  Action pickBestAction(const Sorry &startingState, LoopCondition *loopCondition);
  void doSingleStep(const Sorry &startingState, Node *rootNode);
  int select(const Node *currentNode, bool withExploration, const std::vector<size_t> &indices);
  int rollout(Sorry state);
  void backprop(Node *current, int moveCount);
  double nodeScore(const Node *current, const Node *parent, double maxAverageMoveCount, double minAverageMoveCount, bool withExploration) const;
  void printActions(const Node *current, int levels, int currentLevel=0) const;
};

#endif // SORRY_MCTS_HPP_