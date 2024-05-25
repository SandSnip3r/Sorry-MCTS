#ifndef ACTION_HPP_
#define ACTION_HPP_

#include "card.hpp"

#include <string>

namespace sorry {

class Action {
public:
  enum class ActionType {
    kDiscard,
    kSingleMove,
    kDoubleMove
  };
  static Action discard(Card card);
  static Action singleMove(Card card, int pieceIndex, int moveDestination);
  static Action doubleMove(Card card, int piece1Index, int move1Destination, int piece2Index, int move2Destination);
  std::string toString() const;

  ActionType actionType;
  Card card;
  int piece1Index;
  int move1Destination;
  int piece2Index;
  int move2Destination;
};

bool operator==(const Action &lhs, const Action &rhs);

} // namespace sorry


#endif // ACTION_HPP_