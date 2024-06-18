#ifndef SORRY_MCTS_HPP_
#define SORRY_MCTS_HPP_

#include "action.hpp"

#include <atomic>
#include <chrono>
#include <mutex>
#include <random>
#include <utility>
#include <vector>

class Node;
class LoopCondition;

namespace sorry {
class Sorry;
} // namespace sorry

namespace internal {

class LoopCondition {
public:
  virtual bool condition() const = 0;
  virtual void oneIterationComplete() = 0;
};

} // namespace internal

class ExplicitTerminator : public internal::LoopCondition {
public:
  void setDone(bool done);
  bool condition() const override;
  void oneIterationComplete() override;
private:
  std::atomic<bool> done_{false};
};

struct ActionScore {
  sorry::Action action;
  double score;
};

class SorryMcts {
public:
  explicit SorryMcts(double explorationConstant);
  void run(const sorry::Sorry &startingState, int rolloutCount);
  void run(const sorry::Sorry &startingState, std::chrono::duration<double> timeLimit);
  void run(const sorry::Sorry &startingState, internal::LoopCondition *loopCondition);
  void reset();
  sorry::Action pickBestAction() const;
  std::vector<ActionScore> getActionScores() const;
  std::vector<double> getWinRates() const;
  int getIterationCount() const;
private:
  const double explorationConstant_;
  std::mt19937 eng_{0};
  sorry::PlayerColor ourPlayer_;

  mutable std::mutex treeMutex_;
  Node *rootNode_{nullptr};
  int iterationCount_;
  void doSingleStep(const sorry::Sorry &startingState);

  // Returns the index of the action to take. This is one of the indices in the `indices` vector.
  int select(const Node *currentNode, bool withExploration, const std::vector<size_t> &indices) const;

  sorry::PlayerColor rollout(sorry::Sorry state);
  void backprop(Node *current, sorry::PlayerColor winner);
  double nodeScore(const Node *current, const Node *parent, bool withExploration) const;
  void printActions(const Node *current, int levels, int currentLevel=0) const;
};

#endif // SORRY_MCTS_HPP_