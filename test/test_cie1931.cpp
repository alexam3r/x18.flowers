#include <unity.h>

#include <cmath>

#include "Cie1931.h"

void setUp() {}
void tearDown() {}

void test_endpoints_match_the_12bit_range() {
  TEST_ASSERT_EQUAL_UINT16(0, cie1931::percentToDuty(0.0f));
  TEST_ASSERT_EQUAL_UINT16(4095, cie1931::percentToDuty(100.0f));
}

void test_midpoint_is_close_to_755() {
  const uint16_t duty = cie1931::percentToDuty(50.0f);
  TEST_ASSERT_UINT16_WITHIN(2, 755, duty);
}

void test_curve_is_monotonic() {
  uint16_t previous = 0;
  for (int step = 0; step <= 1000; ++step) {
    const float percent = static_cast<float>(step) / 10.0f;
    const uint16_t duty = cie1931::percentToDuty(percent);
    TEST_ASSERT_TRUE(duty >= previous);
    previous = duty;
  }
}

void test_low_brightness_is_not_linear() {
  // 1 % must be ~5/4095, not 1 % of the duty range.
  TEST_ASSERT_UINT16_WITHIN(1, 5, cie1931::percentToDuty(1.0f));
  // The low segment uses L*/902.3 and joins the cubic curve at 8 %.
  TEST_ASSERT_UINT16_WITHIN(1, 36, cie1931::percentToDuty(8.0f));
}

void test_input_is_clamped() {
  TEST_ASSERT_EQUAL_UINT16(0, cie1931::percentToDuty(-20.0f));
  TEST_ASSERT_EQUAL_UINT16(4095, cie1931::percentToDuty(150.0f));
  TEST_ASSERT_EQUAL_UINT16(0, cie1931::percentToDuty(NAN));
}

void test_inverse_mapping() {
  TEST_ASSERT_FLOAT_WITHIN(0.2f, 50.0f, cie1931::dutyToPercent(754));
  TEST_ASSERT_EQUAL_FLOAT(0.0f, cie1931::dutyToPercent(0));
  TEST_ASSERT_EQUAL_FLOAT(100.0f, cie1931::dutyToPercent(4095));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_endpoints_match_the_12bit_range);
  RUN_TEST(test_midpoint_is_close_to_755);
  RUN_TEST(test_curve_is_monotonic);
  RUN_TEST(test_low_brightness_is_not_linear);
  RUN_TEST(test_input_is_clamped);
  RUN_TEST(test_inverse_mapping);
  return UNITY_END();
}
