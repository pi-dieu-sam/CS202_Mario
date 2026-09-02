// Regression tests for issue #20: persistent score ownership, level
// completion, victory, and retry semantics.

#include "Core/PlayerProgress.hpp"
#include "Core/LevelCompletion.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <iostream>
#include <string>

static int g_failures = 0;

#define CHECK(cond, msg)                                                     \
  do {                                                                       \
    if (!(cond)) {                                                           \
      std::cerr << "FAIL: " << msg << " (" << __FILE__ << ":" << __LINE__    \
                << ")\n";                                                    \
      ++g_failures;                                                          \
    }                                                                        \
  } while (0)

static void testFlagpoleBonusIsPersistedOnce() {
  PlayerProgress progress;
  constexpr int existingScore = 1200;
  constexpr int flagpoleBonus = 5000;

  progress.setScore(existingScore);
  progress.addScore(flagpoleBonus);

  CHECK(progress.getScore() == existingScore + flagpoleBonus,
        "flagpole bonus is added once to persistent progress score");
}

static void testTimeBonusConversion() {
  CHECK(LevelCompletion::displayedSeconds(42.99f) == 42,
        "time bonus uses the HUD's truncated whole-second display");
  CHECK(LevelCompletion::displayedSeconds(-1.0f) == 0,
        "time bonus clamps expired time to zero");
  CHECK(LevelCompletion::timeBonusForSeconds(42) == 42 * TIME_BONUS_PER_SECOND,
        "each remaining second awards the configured score value");

  int remainingSeconds = 3;
  int convertedScore = 0;
  for (int expectedSeconds = 2; expectedSeconds >= 0; --expectedSeconds) {
    CHECK(LevelCompletion::convertNextSecond(remainingSeconds, convertedScore),
          "each remaining second produces one conversion tick");
    CHECK(remainingSeconds == expectedSeconds,
          "conversion tick decreases the displayed time by one second");
    CHECK(convertedScore == (3 - expectedSeconds) * TIME_BONUS_PER_SECOND,
          "conversion tick increases score by the configured amount");
  }
  CHECK(!LevelCompletion::convertNextSecond(remainingSeconds, convertedScore),
        "time conversion stops at zero and cannot award score twice");

  constexpr int flagpoleBonus = 5000;
  int convertedFlagpoleScore = 0;
  int synchronizedSeconds = 3;
  int synchronizedTimeScore = 0;
  for (int expectedSeconds = 2; expectedSeconds >= 0; --expectedSeconds) {
    const int flagpoleIncrement = LevelCompletion::flagpoleBonusForNextTick(
        flagpoleBonus, convertedFlagpoleScore, synchronizedSeconds);
    CHECK(LevelCompletion::convertNextSecond(
              synchronizedSeconds, synchronizedTimeScore),
          "flagpole and time score are applied on the same tick");
    convertedFlagpoleScore += flagpoleIncrement;
    CHECK(synchronizedSeconds == expectedSeconds,
          "synchronized tick decreases time by one second");
  }
  CHECK(convertedFlagpoleScore == flagpoleBonus,
        "synchronized ticks distribute the entire flagpole bonus");
  CHECK(synchronizedTimeScore == 3 * TIME_BONUS_PER_SECOND,
        "synchronized ticks award the full time bonus");
}

static void testOrdinaryCompletionAdvancesLevel() {
  for (int level = 1; level < TOTAL_LEVELS; ++level) {
    PlayerProgress progress;
    progress.setCurrentLevel(level);

    CHECK(progress.advanceToNextLevel(TOTAL_LEVELS),
          "completing a non-final level advances the game");
    CHECK(progress.getCurrentLevel() == level + 1,
          "ordinary completion selects the next level");
  }
}

static void testFinalCompletionReportsVictory() {
  PlayerProgress progress;
  progress.setCurrentLevel(TOTAL_LEVELS);

  CHECK(!progress.advanceToNextLevel(TOTAL_LEVELS),
        "completing the final level reports victory instead of advancing");
  CHECK(progress.getCurrentLevel() == TOTAL_LEVELS,
        "victory leaves the completed final level selected");
}

static void testRetryPreservesProgressForEveryLevel() {
  for (int level = 1; level <= TOTAL_LEVELS; ++level) {
    PlayerProgress progress;
    progress.setCurrentLevel(level);
    progress.setScore(4000 + level);
    progress.setCoins(10 + level);
    progress.setSelectedCharacter("Luigi");
    progress.setLives(0);

    progress.retryCurrentLevel();

    CHECK(progress.getCurrentLevel() == level,
          "retry keeps the failed level selected");
    CHECK(progress.getScore() == 4000 + level,
          "retry preserves the accumulated score");
    CHECK(progress.getCoins() == 10 + level,
          "retry preserves collected coins");
    CHECK(progress.getSelectedCharacter() == "Luigi",
          "retry preserves the selected character");
    CHECK(progress.getLives() == STARTING_LIVES,
          "retry restores the starting life count");
  }
}

static void testAddCoinIncreasesCoinsAndScore() {
  PlayerProgress progress;
  constexpr int coinsToCollect = 37; // stays under the 100-coin extra-life threshold

  for (int i = 0; i < coinsToCollect; ++i) {
    progress.addCoin();
  }

  CHECK(progress.getCoins() == coinsToCollect,
        "each addCoin() call increases the coin count by exactly one");
  CHECK(progress.getScore() == coinsToCollect * COIN_SCORE,
        "each collected coin awards the configured coin score");
}

static void testHundredCoinsAwardsExactlyOneExtraLife() {
  PlayerProgress progress;
  progress.setLives(STARTING_LIVES);
  const int baseline = progress.getLives();

  for (int i = 0; i < 99; ++i) {
    progress.addCoin();
  }
  CHECK(progress.getLives() == baseline,
        "the 99th coin does not yet award an extra life");

  progress.addCoin(); // 100th coin
  CHECK(progress.getLives() == baseline + 1,
        "exactly the 100th coin awards one extra life");

  for (int i = 0; i < 99; ++i) {
    progress.addCoin();
  }
  CHECK(progress.getLives() == baseline + 1,
        "the 199th coin still has not awarded a second extra life");

  progress.addCoin(); // 200th coin
  CHECK(progress.getLives() == baseline + 2,
        "the 200th coin awards a second extra life");
}

static void testLoseLifeRemovesExactlyOneLifeAndFloorsAtZero() {
  PlayerProgress progress;
  progress.setLives(3);

  progress.loseLife();
  CHECK(progress.getLives() == 2, "a single death removes exactly one life");

  progress.loseLife();
  CHECK(progress.getLives() == 1, "each subsequent death also removes one life");

  progress.loseLife();
  CHECK(progress.getLives() == 0, "the final life drops the count to zero");

  progress.loseLife();
  CHECK(progress.getLives() == 0,
        "losing a life at zero lives does not go negative");
}

static void testNewGameRemainsFullReset() {
  PlayerProgress progress;
  progress.setCurrentLevel(TOTAL_LEVELS);
  progress.setScore(9999);
  progress.setCoins(42);
  progress.setLives(1);
  progress.setSelectedCharacter("Luigi");

  progress.resetGameData();

  CHECK(progress.getCurrentLevel() == 1, "new game resets to level 1");
  CHECK(progress.getScore() == 0, "new game resets score");
  CHECK(progress.getCoins() == 0, "new game resets coins");
  CHECK(progress.getLives() == STARTING_LIVES, "new game resets lives");
  CHECK(progress.getSelectedCharacter() == "Mario",
        "new game restores the default character before character selection");
}

int main() {
  testFlagpoleBonusIsPersistedOnce();
  testTimeBonusConversion();
  testOrdinaryCompletionAdvancesLevel();
  testFinalCompletionReportsVictory();
  testRetryPreservesProgressForEveryLevel();
  testAddCoinIncreasesCoinsAndScore();
  testHundredCoinsAwardsExactlyOneExtraLife();
  testLoseLifeRemovesExactlyOneLifeAndFloorsAtZero();
  testNewGameRemainsFullReset();

  if (g_failures == 0) {
    std::cout << "All progression tests passed.\n";
    return 0;
  }

  std::cerr << g_failures << " test(s) failed.\n";
  return 1;
}
