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

class Deck {
public:
  void initialize();
  void removeSpecificCard(Card card);
  Card drawRandomCard(std::mt19937 &eng);
  size_t size() const;
  bool empty() const;
private:
  using CardsType = std::array<Card, 45>;
  CardsType cards_;
  size_t size_{0};
  void removeCard(CardsType::iterator it);

  friend bool operator==(const Deck &lhs, const Deck &rhs);
};

class Sorry {
public:
  Sorry();
  void drawRandomStartingCards(std::mt19937 &eng);
  void setStartingCards(std::array<Card,5> cards);
  void setStartingPositions(const std::array<int, 4> &positions);
  std::string toString() const;
  std::string handToString() const;
  std::vector<Action> getActions() const;
  void doAction(const Action &action, std::mt19937 &eng);
  bool gameDone() const;
  int getTotalActionCount() const;
private:
  Deck deck_;
  std::array<Card,5> hand_;
  bool haveStartingHand_{false};
  std::array<int, 4> piecePositions_ = {2,0,0,0}; // Always start the game with one piece just outside of Start.
  int actionCount_{0};
  void drawRandomCardIntoIndex(int index, std::mt19937 &eng);
  void addActionsForCard(Card card, std::vector<Action> &actions) const;
  std::optional<int> getMoveResultingPos(int pieceIndex, int moveDistance) const;
  std::optional<std::pair<int,int>> getDoubleMoveResultingPos(int piece1Index, int move1Distance, int piece2Index, int move2Distance) const;
  int slideLengthAtPos(int pos) const;
  int posAfterSlide(int pos) const;
  size_t indexOfCardInHand(Card card) const;

  friend bool operator==(const Sorry &lhs, const Sorry &rhs);
};


#endif // SORRY_HPP_