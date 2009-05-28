// a parser for fnmatch rules


#include "fnmatch_parse.h"

#include <string>
#include <vector>


std::vector<FnmatchRule*> 
FnmatchParser::Parse(const std::string& rule) {
  std::vector<FnmatchRule*> rules;

  // iterate through the string...
  for (std::string::const_iterator i = rule.begin(); i < rule.end(); i++) {
    rules.push_back(ParseOne(i, rule.end()));
  }

  return rules;
}


FnmatchRule*
FnmatchParser::ParseOne(std::string::const_iterator& i, const std::string::const_iterator& end) {
  // parse one rule
  switch(*i) {
    case '\\':
      // escape character...
      i++;
      if (i >= end) {
        // out of characters. bugger.
        throw std::string("parse error - escape at end of string");
      }
      return new FnmatchCharacter(*i);
    case '?':
      return new FnmatchSingle();
    case '*':
      return new FnmatchMultiple();
    case '[':
      {
        i++;
        if (i >= end) {
          // out of characters. bugger.
          throw std::string("parse error - bracket at end of string");
        }
        bool inverse = (*i == '!');
        if (inverse) {
          i++;
        }
        std::string characters;
        while (i<end) {
          if (*i == ']') {
            return new FnmatchBracket(inverse, characters);
          } else {
            characters += *i;
          }
        }
        throw std::string("parse error - unterminated bracket expression");
      }
    default:
      return new FnmatchCharacter(*i);
  }
}

