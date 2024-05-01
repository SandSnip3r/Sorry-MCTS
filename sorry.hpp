#ifndef SORRY_HPP_
#define SORRY_HPP_

#include "action.hpp"
#include "card.hpp"

#include <array>
#include <optional>
#include <random>
#include <string>
#include <utility>
#include <vector>

class Sorry {
public:
  Sorry();
  void drawRandomStartingCards(std::mt19937 &eng);
  void setStartingCards(const std::array<Card,5> &cards);
  void setStartingPositions(const std::array<int, 4> &positions);
  std::string toString() const;
  std::vector<Action> getActions() const;
  void doAction(const Action &action, std::mt19937 &eng);
  bool gameDone() const;
  int getTotalActionCount() const;
private:
  std::vector<Card> deck_;
  std::array<Card,5> hand_;
  bool haveStartingHand_{false};
  std::array<int, 4> piecePositions_ = {0,0,0,0};
  int actionCount_{0};
  void drawRandomCardIntoIndex(int index, std::mt19937 &eng);
  void fillDeck();
  void addActionsForCard(int cardIndex, std::vector<Action> &actions) const;
  std::optional<int> getMoveResultingPos(int pieceIndex, int moveDistance) const;
  std::optional<std::pair<int,int>> getDoubleMoveResultingPos(int piece1Index, int move1Distance, int piece2Index, int move2Distance) const;
  int slideLengthAtPos(int pos) const;
  int posAfterSlide(int pos) const;
};

#endif // SORRY_HPP_