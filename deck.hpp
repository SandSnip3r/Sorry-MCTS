#ifndef DECK_HPP_
#define DECK_HPP_

#include "card.hpp"

#include <array>
#include <random>

namespace sorry {

class Deck {
public:
  Deck() { initialize(); }
  void initialize();
  void removeSpecificCard(Card card);
  Card drawRandomCard(std::mt19937 &eng);
  void discard(Card card);
  size_t size() const;
  bool empty() const;
  void shuffle();

  bool equalDiscarded(const Deck &other) const;
private:
  std::array<Card, 45> cards_;
  size_t firstOutIndex_;
  size_t firstDiscardIndex_;
  void removeCard(size_t index);
  void print() const;

  friend class Sorry;
  Card drawRandomCardAlsoFromOut(std::mt19937 &eng);
};

} // namespace sorry

#endif // DECK_HPP_