//
// This is just to keep all of the "magic numbers" in one place
// Ideally these will all be exposed to an eventual user
//

#pragma once

#include <ratio>

// STACK_SIZE is independant from ANTE, ie STACK_SIZE is the remaining chips that can go in play
using ANTE = std::ratio<1, 1>;
using STACK_SIZE = std::ratio<10, 1>;
