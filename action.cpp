#include "action.hpp"

#include <sstream>
#include <stdexcept>

Action Action::discard(int cardIndex) {
  Action a;
  a.actionType = ActionType::kDiscard;
  a.cardIndex = cardIndex;
  return a;
}

Action Action::singleMove(int cardIndex, int pieceIndex, int moveDestination) {
  Action a;
  a.actionType = ActionType::kSingleMove;
  a.cardIndex = cardIndex;
  a.piece1Index = pieceIndex;
  a.move1Destination = moveDestination;
  return a;
}

Action Action::doubleMove(int cardIndex, int piece1Index, int move1Destination, int piece2Index, int move2Destination) {
  Action a;
  a.actionType = ActionType::kDoubleMove;
  a.cardIndex = cardIndex;
  a.piece1Index = piece1Index;
  a.move1Destination = move1Destination;
  a.piece2Index = piece2Index;
  a.move2Destination = move2Destination;
  return a;
}

std::string Action::toString() const {
  std::stringstream ss;
  if (actionType == ActionType::kDiscard) {
    ss << "Discard";
  } else if (actionType == ActionType::kSingleMove) {
    ss << "SingleMove";
  } else if (actionType == ActionType::kDoubleMove) {
    ss << "DoubleMove";
  } else {
    throw std::runtime_error("Unknown action type");
  }
  ss << ',' << cardIndex;
  if (actionType != ActionType::kDiscard) {
    ss << ',' << piece1Index << ',' << move1Destination;
  }
  if (actionType == ActionType::kDoubleMove) {
    ss << ',' << piece2Index << ',' << move2Destination;
  }
  return ss.str();
}

bool operator==(const Action &lhs, const Action &rhs) {
  if (lhs.actionType == Action::ActionType::kDiscard) {
    return rhs.actionType == Action::ActionType::kDiscard &&
           lhs.cardIndex == rhs.cardIndex;
  } else if (lhs.actionType == Action::ActionType::kSingleMove) {
    return rhs.actionType == Action::ActionType::kSingleMove &&
           lhs.cardIndex == rhs.cardIndex &&
           lhs.piece1Index == rhs.piece1Index &&
           lhs.move1Destination == rhs.move1Destination;
  } else {
    return rhs.actionType == Action::ActionType::kDoubleMove &&
           lhs.cardIndex == rhs.cardIndex &&
           lhs.piece1Index == rhs.piece1Index &&
           lhs.move1Destination == rhs.move1Destination &&
           lhs.piece2Index == rhs.piece2Index &&
           lhs.move2Destination == rhs.move2Destination;
  }
}