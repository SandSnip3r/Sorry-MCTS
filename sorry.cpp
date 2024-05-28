#include "sorry.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace sorry {

void Deck::initialize() {
  // 5 one's
  // 4 of every other card (2's, 3's, 4's, 5's, 7's, 8's, 10's, 11's, 12's,)
  // 4 Sorry cards
  cards_ = { Card::kOne,    Card::kOne,    Card::kOne,    Card::kOne,    Card::kOne,
             Card::kTwo,    Card::kTwo,    Card::kTwo,    Card::kTwo,
             Card::kThree,  Card::kThree,  Card::kThree,  Card::kThree,
             Card::kFour,   Card::kFour,   Card::kFour,   Card::kFour,
             Card::kFive,   Card::kFive,   Card::kFive,   Card::kFive,
             Card::kSeven,  Card::kSeven,  Card::kSeven,  Card::kSeven,
             Card::kEight,  Card::kEight,  Card::kEight,  Card::kEight,
             Card::kTen,    Card::kTen,    Card::kTen,    Card::kTen,
             Card::kEleven, Card::kEleven, Card::kEleven, Card::kEleven,
             Card::kTwelve, Card::kTwelve, Card::kTwelve, Card::kTwelve,
             Card::kSorry,  Card::kSorry,  Card::kSorry,  Card::kSorry };
  size_ = cards_.size();
}

void Deck::removeSpecificCard(Card card) {
  auto it = std::find(cards_.begin(), cards_.end(), card);
  if (it == cards_.end()) {
    throw std::runtime_error("Card not found in deck");
  }
  removeCard(it);
}

Card Deck::drawRandomCard(std::mt19937 &eng) {
  std::uniform_int_distribution<int> dist(0, size_-1);
  const int drawnCardIndex = dist(eng);
  Card card = cards_[drawnCardIndex];
  removeCard(cards_.begin() + drawnCardIndex);
  return card;
}

size_t Deck::size() const {
  return size_;
}

bool Deck::empty() const {
  return size_ == 0;
}

void Deck::removeCard(CardsType::iterator it) {
  std::iter_swap(it, cards_.begin() + size_-1);
  --size_;
}

bool operator==(const sorry::Deck &lhs, const sorry::Deck &rhs) {
  if (lhs.size_ != rhs.size_) {
    return false;
  }
  for (size_t i=0; i<lhs.size_; ++i) {
    if (lhs.cards_[i] != rhs.cards_[i]) {
      return false;
    }
  }
  return true;
}

Sorry::Sorry() {
  deck_.initialize();
}

void Sorry::drawRandomStartingCards(std::mt19937 &eng) {
  for (size_t i=0; i<hand_.size(); ++i) {
    drawRandomCardIntoIndex(i, eng);
  }
  haveStartingHand_ = true;
}

void Sorry::setStartingCards(std::array<Card,5> cards) {
  // Remove cards from deck and insert into hand.
  for (size_t i=0; i<cards.size(); ++i) {
    hand_[i] = cards[i];
    deck_.removeSpecificCard(cards[i]);
  }
  haveStartingHand_ = true;
}

void Sorry::setStartingPositions(const std::array<int, 4> &positions) {
  for (size_t i=0; i<positions.size(); ++i) {
    if (slideLengthAtPos(positions[i]) != 0) {
      throw std::runtime_error("Cannot start with piece on start of slide"); // TODO: Actually not true in multiplayer.
    }
    piecePositions_[i] = positions[i];
  }
}

std::string Sorry::toString() const {
  if (!haveStartingHand_) {
    throw std::runtime_error("Called toString() without a starting hand set");
  }
  std::stringstream ss;
  ss << '{';
  // Stringify hand
  ss << "Hand:" << handToString();
  // Stringify deck
  ss << ";deck:" << deck_.size();
  // Stringify piece positions
  ss << ";piece positions(";
  for (int i=0; i<4; ++i) {
    ss << piecePositions_[i];
    if (i != 3) {
      ss << ',';
    }
  }
  ss << ")";
  ss << '}';
  return ss.str();
}

std::string Sorry::handToString() const {
  if (!haveStartingHand_) {
    throw std::runtime_error("Called handToString() without a starting hand set");
  }
  std::stringstream ss;
  for (int i=0; i<5; ++i) {
    ss << sorry::toString(hand_[i]);
    if (i != 4) {
      ss << ',';
    }
  }
  return ss.str();
}

std::vector<Action> Sorry::getActions() const {
  if (!haveStartingHand_) {
    throw std::runtime_error("Called getActions() without a starting hand set");
  }
  if (gameDone()) {
    return {};
  }
  std::vector<Action> result;
  result.reserve(60); // Save some time by doing one allocation large enough for most invocations.
  for (size_t i=0; i<hand_.size(); ++i) {
    bool alreadyHandledThisCard = false;
    for (int j=static_cast<int>(i)-1; j>=0; --j) {
      if (hand_[j] == hand_[i]) {
        // Already handled one of these cards.
        alreadyHandledThisCard = true;
      }
    }
    if (alreadyHandledThisCard) {
      continue;
    }
    addActionsForCard(hand_[i], result);
  }

  if (result.empty()) {
    // If no options, create successor states for discarding.
    for (size_t i=0; i<hand_.size(); ++i) {
      bool alreadyDiscarded=false;
      for (size_t j=0; j<i; ++j) {
        if (hand_[i] == hand_[j]) {
          // Already discarded one of these.
          alreadyDiscarded = true;
          break;
        }
      }
      if (!alreadyDiscarded) {
        result.push_back(Action::discard(hand_[i]));
      }
    }
  }
  return result;
}

void Sorry::drawRandomCardIntoIndex(int index, std::mt19937 &eng) {
  hand_[index] = deck_.drawRandomCard(eng);

  if (deck_.empty()) {
    deck_.initialize();
    setStartingCards(hand_);
  }
}

void Sorry::doAction(const Action &action, std::mt19937 &eng) {
  if (!haveStartingHand_) {
    throw std::runtime_error("Called doAction() without a starting hand set");
  }
  if (action.actionType != Action::ActionType::kDiscard) {
    // Move one or two pieces
    piecePositions_[action.piece1Index] = posAfterSlide(action.move1Destination);

    if (action.actionType == Action::ActionType::kDoubleMove) {
      piecePositions_[action.piece2Index] = posAfterSlide(action.move2Destination);
    }
  }

  // Finally. Discard card.
  drawRandomCardIntoIndex(indexOfCardInHand(action.card), eng);
  ++actionCount_;

  // At the end, do a quick sanity check to make that no two pieces are in the same spot, apart from start and home.
  for (size_t i=0; i<piecePositions_.size(); ++i) {
    for (size_t j=i+1; j<piecePositions_.size(); ++j) {
      if (piecePositions_[i] == piecePositions_[j] &&
          piecePositions_[i] != 0 &&
          piecePositions_[i] != 66) {
        std::cout << "State: " << toString() << std::endl;
        std::cout << "Action: " << action.toString() << std::endl;
        throw std::runtime_error("Piece "+std::to_string(i)+" and "+std::to_string(j)+" are in the same spot");
      }
    }
  }
}

bool Sorry::gameDone() const {
  for (size_t i=0; i<piecePositions_.size(); ++i) {
    if (piecePositions_[i] != 66) {
      return false;
    }
  }
  return true;
}

int Sorry::getTotalActionCount() const {
  return actionCount_;
}

std::array<Card,5> Sorry::getHand() const {
  return hand_;
}

std::array<int, 4> Sorry::getPiecePositions() const {
  return piecePositions_;
}

void Sorry::addActionsForCard(Card card, std::vector<Action> &actions) const {
  auto tryAddMoveToAllPositions = [this, &actions](Card card, int moveAmount) {
    for (size_t pieceIndex=0; pieceIndex<piecePositions_.size(); ++pieceIndex) {
      auto moveResult = getMoveResultingPos(pieceIndex, moveAmount);
      if (moveResult) {
        // Use card `card` and move piece `pieceIndex` from `piecePositions_[pieceIndex]` to `*moveResult`
        actions.push_back(Action::singleMove(card, pieceIndex, *moveResult));
      }
    }
  };
  // Add the state from simply moving forward
  {
    int moveAmount;
    if (card == Card::kFour) {
      moveAmount = -4;
    } else {
      moveAmount = static_cast<int>(card);
    }

    // Note: This does not produce any duplicate actions because no two pieces can be in the same position (moving out of home is not handled here).
    tryAddMoveToAllPositions(card, moveAmount);
  }

  // Move piece out of start, if possible.
  if (card == Card::kOne || card == Card::kTwo) {
    // Is any piece already on the start position?
    bool canMoveOutOfHome = true;
    for (auto position : piecePositions_) {
      if (position == 2) {
        canMoveOutOfHome = false;
        break;
      }
    }
    if (canMoveOutOfHome) {
      for (size_t pieceIndex=0; pieceIndex<piecePositions_.size(); ++pieceIndex) {
        if (piecePositions_[pieceIndex] == 0) {
          // This piece is in start.
          actions.push_back(Action::singleMove(card, pieceIndex, 2));
          // Note: Breaking after moving one item from start prevents duplicate actions. Any other piece in start results in the same action and pieces not in start don't apply here.
          break;
        }
      }
    }
  }

  if (card == Card::kTen) {
    // 10 can also go backward 1.
    tryAddMoveToAllPositions(card, -1);
  }
  if (card == Card::kSeven) {
    // 7 can have the 7 split across two pieces
    for (int move1=4; move1<7; ++move1) {
      int move2 = 7-move1;
      for (size_t piece1Index=0; piece1Index<piecePositions_.size(); ++piece1Index) {
        for (size_t piece2Index=0; piece2Index<piecePositions_.size(); ++piece2Index) {
          if (piece1Index == piece2Index) {
            continue;
          }
          auto doubleMoveResult = getDoubleMoveResultingPos(piece1Index, move1, piece2Index, move2);
          if (doubleMoveResult) {
            // Use card `card` and move piece `piece1Index` from `piecePositions_[piece1Index]` to `doubleMoveResult->first` and move piece `piece2Index` from `piecePositions_[piece2Index]` to `doubleMoveResult->second`.
            actions.push_back(Action::doubleMove(card, piece1Index, doubleMoveResult->first, piece2Index, doubleMoveResult->second));
          }
        }
      }
    }
  }
  // TODO: Handle 11-swap
  // TODO: Handle Sorry
}

std::optional<int> Sorry::getMoveResultingPos(int pieceIndex, int moveDistance) const {
  const int startingPosition = piecePositions_[pieceIndex];
  if (startingPosition == 0) {
    // Is in start. Can't move.
    return {};
  }
  if (startingPosition == 66) {
    // Is in home. Can't move.
    return {};
  }
  // Valid positions are 0-66. 0 is start, 66 is home.
  if (startingPosition+moveDistance > 66) {
    // Cannot go beyond home.
    return {};
  }
  // 5 safe positions (65,64,63,62,61)
  int newPos = startingPosition + moveDistance;
  if (newPos < 1) {
    // Wrap around.
    // 0->60
    // -1->59
    newPos += 60;
  }
  if (newPos == startingPosition) {
    throw std::runtime_error("New position == starting position");
  }
  if (newPos == 66) {
    return newPos;
  }

  // Do we land on one of our own pieces?
  for (int i=0; i<4; ++i) {
    if (newPos == piecePositions_[i]) {
      // Cannot move here.
      return {};
    }
  }
  int slideLength = slideLengthAtPos(newPos);
  for (int slidePos=0; slidePos<slideLength; ++slidePos) {
    for (size_t otherPieceIndex=0; otherPieceIndex<piecePositions_.size(); ++otherPieceIndex) {
      if (static_cast<int>(otherPieceIndex) == pieceIndex) {
        // Do not check collision with self.
        continue;
      }
      if (piecePositions_[otherPieceIndex] == newPos + slidePos) {
        // One of our pieces is on this slide. Cannot move here.
        return {};
      }
    }
  }
  return newPos;
}

std::optional<std::pair<int,int>> Sorry::getDoubleMoveResultingPos(int piece1Index, int move1Distance, int piece2Index, int move2Distance) const {
  const int startingPosition1 = piecePositions_[piece1Index];
  const int startingPosition2 = piecePositions_[piece2Index];
  if (startingPosition1 == 0 || startingPosition2 == 0) {
    // Is in start. Can't move.
    return {};
  }
  if (startingPosition1 == 66 || startingPosition2 == 66) {
    // Is in home. Can't move.
    return {};
  }
  // Valid positions are 0-66. 0 is start, 66 is home.
  if (startingPosition1+move1Distance > 66 || startingPosition2+move2Distance > 66) {
    // Cannot go beyond home.
    return {};
  }
  // 5 safe positions (65,64,63,62,61)
  int newPos1 = startingPosition1 + move1Distance;
  int newPos2 = startingPosition2 + move2Distance;
  if (newPos1 == newPos2 && newPos1 != 66) {
    // Cannot move both pieces to the same place.
    return {};
  }
  if (newPos1 < 1) {
    // Wrap around.
    // 0->60
    // -1->59
    newPos1 += 60;
  }
  if (newPos2 < 1) {
    // Wrap around.
    // 0->60
    // -1->59
    newPos2 += 60;
  }

  // Do we land on one of our own pieces?
  for (int i=0; i<4; ++i) {
    if (i == piece1Index || i == piece2Index) {
      // Do not check collision with moving pieces.
      continue;
    }
    if ((newPos1 != 66 && piecePositions_[i] == newPos1) ||
        (newPos2 != 66 && piecePositions_[i] == newPos2)) {
      // Cannot move here.
      return {};
    }
  }

  // Check if either of these pieces slide over one of our other non-moving pieces.
  int slideLength1 = slideLengthAtPos(newPos1);
  for (int slidePos=0; slidePos<slideLength1; ++slidePos) {
    for (size_t otherPieceIndex=0; otherPieceIndex<piecePositions_.size(); ++otherPieceIndex) {
      if (static_cast<int>(otherPieceIndex) == piece1Index || static_cast<int>(otherPieceIndex) == piece2Index) {
        // Do not check collision with self.
        continue;
      }
      if (piecePositions_[otherPieceIndex] == newPos1 + slidePos) {
        // One of our pieces is on this slide. Cannot move here.
        return {};
      }
    }
  }
  int slideLength2 = slideLengthAtPos(newPos2);
  for (int slidePos=0; slidePos<slideLength2; ++slidePos) {
    for (size_t otherPieceIndex=0; otherPieceIndex<piecePositions_.size(); ++otherPieceIndex) {
      if (static_cast<int>(otherPieceIndex) == piece1Index || static_cast<int>(otherPieceIndex) == piece2Index) {
        // Do not check collision with self.
        continue;
      }
      if (piecePositions_[otherPieceIndex] == newPos2 + slidePos) {
        // One of our pieces is on this slide. Cannot move here.
        return {};
      }
    }
  }

  // Check if one piece is on a slide and if the other one is going to slide on it.
  if (slideLength1 > 0) {
    // Piece 1 is going to slide, check if piece 2 is currently on the slide.
    bool inTheWayBefore{false};
    bool inTheWayAfter{false};
    for (int slidePos=0; slidePos<slideLength1; ++slidePos) {
      if (startingPosition2 == newPos1 + slidePos) {
        // Piece 2 is in our way before it moves.
        inTheWayBefore = true;
      }
      if (newPos2 == newPos1 + slidePos) {
        // Piece 2 is in our way after it moves.
        inTheWayAfter = true;
      }
    }
    if (inTheWayBefore && inTheWayAfter) {
      return {};
    }
  }

  if (slideLength2 > 0) {
    // Piece 2 is going to slide, check if piece 1 is currently on the slide.
    bool inTheWayBefore{false};
    bool inTheWayAfter{false};
    for (int slidePos=0; slidePos<slideLength2; ++slidePos) {
      if (startingPosition1 == newPos2 + slidePos) {
        // Piece 1 is in our way before it moves.
        inTheWayBefore = true;
      }
      if (newPos1 == newPos2 + slidePos) {
        // Piece 1 is in our way after it moves.
        inTheWayAfter = true;
      }
    }
    if (inTheWayBefore && inTheWayAfter) {
      return {};
    }
  }

  const int afterSlide1 = posAfterSlide(newPos1);
  const int afterSlide2 = posAfterSlide(newPos2);
  if (afterSlide1 == afterSlide2 && afterSlide1 != 66) {
    // Both end at the same spot. Not acceptable.
    return {};
  }
  return std::make_pair(newPos1, newPos2);
}

int Sorry::slideLengthAtPos(int pos) const {
  // Do we land on a slide? 59 is our right slide. 7 is our left slide.
  // 14 is next player's first slide, 22 is second
  // 29 is next player's first slide, 37 is second
  // 44 is next player's first slide, 52 is second
  // First slide is length 4, second is 5
  int slideLength = 0;
  if (pos == 14 || pos == 29 || pos == 44) {
    slideLength = 4;
  } else if (pos == 22 || pos == 37 || pos == 52) {
    slideLength = 5;
  }
  return slideLength;
}

int Sorry::posAfterSlide(int pos) const {
  int slideLength = slideLengthAtPos(pos);
  if (slideLength > 0) {
    return pos + (slideLength-1);
  }
  return pos;
}

size_t Sorry::indexOfCardInHand(Card card) const {
  // Find the index of this card.
  for (size_t i=0; i<hand_.size(); ++i) {
    if (hand_[i] == card) {
      return i;
    }
  }
  throw std::runtime_error("Do not have card "+sorry::toString(card)+" in hand");
}

bool operator==(const sorry::Sorry &lhs, const sorry::Sorry &rhs) {
  for (size_t i=0; i<lhs.hand_.size(); ++i) {
    if (lhs.hand_[i] != rhs.hand_[i]) {
      return false;
    }
  }
  for (size_t i=0; i<lhs.piecePositions_.size(); ++i) {
    if (lhs.piecePositions_[i] != rhs.piecePositions_[i]) {
      return false;
    }
  }
  return lhs.deck_ == rhs.deck_;
}

} // namespace sorry