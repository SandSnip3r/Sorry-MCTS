#ifndef CARD_HPP_
#define CARD_HPP_

#include <string>

enum class Card {
  kOne = 1,
  kTwo = 2,
  kThree = 3,
  kFour = 0,
  kFive = 5,
  kSeven = 7,
  kEight = 8,
  kTen = 10,
  kEleven = 11,
  kTwelve = 12,
  kSorry = 4,
};

std::string toString(Card c);

#endif // CARD_HPP_