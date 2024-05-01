#ifndef ACTION_HPP_
#define ACTION_HPP_

#include <string>

class Action {
public:
  enum class ActionType {
    kDiscard,
    kSingleMove,
    kDoubleMove
  };
  static Action discard(int cardIndex);
  static Action singleMove(int cardIndex, int pieceIndex, int moveDestination);
  static Action doubleMove(int cardIndex, int piece1Index, int move1Destination, int piece2Index, int move2Destination);
  std::string toString() const;

  ActionType actionType;
  int cardIndex;
  int piece1Index;
  int move1Destination;
  int piece2Index;
  int move2Destination;
};

bool operator==(const Action &lhs, const Action &rhs);

#endif // ACTION_HPP_