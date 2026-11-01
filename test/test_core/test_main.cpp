// Host-side unit tests for the hardware-independent core: `pio test -e native`.
#include <unity.h>

#include <cstring>

#include "geo_format.h"
#include "motion_detector.h"
#include "ring_buffer.h"
#include "timing.h"
#include "track_point.h"

using namespace tracker;

void setUp() {}
void tearDown() {}

// --- coordinates ----------------------------------------------------------

static void test_degrees_minutes_basic() {
    // 50.2538° = 50° 15.228'
    const DegreesMinutes dm = toDegreesMinutes(50.2538, true);
    TEST_ASSERT_EQUAL_UINT16(50, dm.degrees);
    TEST_ASSERT_EQUAL_UINT8(15, dm.minutes);
    TEST_ASSERT_EQUAL_UINT8(23, dm.hundredths);
    TEST_ASSERT_EQUAL_CHAR('N', dm.hemisphere);
}

static void test_degrees_minutes_rounding_carries_into_degrees() {
    // 59.9999' rounds to the next whole degree (00.00'), not clamped to 59.99'.
    const DegreesMinutes dm = toDegreesMinutes(-(18.0 + 59.9999 / 60.0), false);
    TEST_ASSERT_EQUAL_UINT16(19, dm.degrees);
    TEST_ASSERT_EQUAL_UINT8(0, dm.minutes);
    TEST_ASSERT_EQUAL_UINT8(0, dm.hundredths);
    TEST_ASSERT_EQUAL_CHAR('W', dm.hemisphere);
}

static void test_degrees_minutes_rounds_to_nearest_not_up() {
    // 12.221' is 12.22', not 12.23' (the first version always rounded up).
    const DegreesMinutes dm = toDegreesMinutes(10.0 + 12.221 / 60.0, true);
    TEST_ASSERT_EQUAL_UINT8(12, dm.minutes);
    TEST_ASSERT_EQUAL_UINT8(22, dm.hundredths);
}

static void test_hemispheres() {
    TEST_ASSERT_EQUAL_CHAR('S', toDegreesMinutes(-33.9, true).hemisphere);
    TEST_ASSERT_EQUAL_CHAR('E', toDegreesMinutes(0.0, false).hemisphere);
}

static void test_coordinate_strings_have_fixed_width() {
    char lat[20];
    char lng[20];
    formatLatitude(lat, sizeof lat, 50.2538);
    formatLongitude(lng, sizeof lng, -61.0345);
    TEST_ASSERT_EQUAL_STRING(" 50  15.23' N", lat);
    TEST_ASSERT_EQUAL_STRING("061  02.07' W", lng);
    TEST_ASSERT_EQUAL(strlen(kNoLatitude), strlen(lat));
    TEST_ASSERT_EQUAL(strlen(kNoLongitude), strlen(lng));
}

// --- speed and course -------------------------------------------------------

static void test_speed_rounds_to_tenths_and_clamps() {
    char buf[12];
    formatSpeed(buf, sizeof buf, 5.26f);
    TEST_ASSERT_EQUAL_STRING("05.3KT", buf);
    formatSpeed(buf, sizeof buf, 0.0f);
    TEST_ASSERT_EQUAL_STRING("00.0KT", buf);
    formatSpeed(buf, sizeof buf, 250.0f);
    TEST_ASSERT_EQUAL_STRING("99.9KT", buf);
}

static void test_course_wraps_at_360() {
    char buf[8];
    formatCourse(buf, sizeof buf, 7.4f);
    TEST_ASSERT_EQUAL_STRING("007", buf);
    formatCourse(buf, sizeof buf, 359.6f);
    TEST_ASSERT_EQUAL_STRING("000", buf);
}

// --- motion detection --------------------------------------------------------

static void test_motion_needs_continuous_movement_to_start() {
    MotionDetector motion(6000, 3000);
    motion.update(true, 0);
    motion.update(true, 5999);
    TEST_ASSERT_FALSE(motion.isMoving());
    motion.update(true, 6000);
    TEST_ASSERT_TRUE(motion.isMoving());
}

static void test_motion_glitch_resets_the_start_timer() {
    MotionDetector motion(6000, 3000);
    motion.update(true, 0);
    motion.update(false, 4000);  // one fix without heading
    motion.update(true, 5000);
    motion.update(true, 10000);  // only 5 s of continuous movement
    TEST_ASSERT_FALSE(motion.isMoving());
    motion.update(true, 11000);
    TEST_ASSERT_TRUE(motion.isMoving());
}

static void test_motion_stops_after_continuous_absence() {
    MotionDetector motion(0, 3000);
    motion.update(true, 100);
    TEST_ASSERT_TRUE(motion.isMoving());
    motion.update(false, 1000);
    motion.update(false, 3999);
    TEST_ASSERT_TRUE(motion.isMoving());
    motion.update(false, 4000);
    TEST_ASSERT_FALSE(motion.isMoving());
}

static void test_motion_survives_millis_wraparound() {
    MotionDetector motion(6000, 3000);
    motion.update(true, 0xFFFFF000u);  // ~4 s before the 32-bit counter wraps
    motion.update(true, 0x00000900u);  // ~6.4 s later, after the wrap
    TEST_ASSERT_TRUE(motion.isMoving());
}

// --- timers and button --------------------------------------------------------

static void test_interval_timer_fires_first_then_per_period() {
    IntervalTimer timer(3000);
    TEST_ASSERT_TRUE(timer.due(100));
    TEST_ASSERT_FALSE(timer.due(3099));
    TEST_ASSERT_TRUE(timer.due(3100));
}

static void test_button_reports_one_press_per_push() {
    ButtonDebouncer button(30);
    TEST_ASSERT_FALSE(button.pressed(true, 0));    // bounce starts
    TEST_ASSERT_FALSE(button.pressed(false, 5));   // bounce
    TEST_ASSERT_FALSE(button.pressed(true, 10));
    TEST_ASSERT_FALSE(button.pressed(true, 30));   // not settled yet
    TEST_ASSERT_TRUE(button.pressed(true, 40));    // settled -> one press
    TEST_ASSERT_FALSE(button.pressed(true, 2000)); // held down: no repeat
    TEST_ASSERT_FALSE(button.pressed(false, 2100));
    TEST_ASSERT_FALSE(button.pressed(false, 2200));
    TEST_ASSERT_FALSE(button.pressed(true, 2300));
    TEST_ASSERT_TRUE(button.pressed(true, 2400));  // second push
}

// --- logging policy and CSV ----------------------------------------------------

static FixQuality goodFix() { return FixQuality{true, true, true, 4.0f, 6.0f}; }

static void test_should_log_requires_position_time_and_accuracy() {
    const LoggingPolicy policy{15.0f, false};
    TEST_ASSERT_TRUE(shouldLog(goodFix(), false, policy));

    FixQuality noTime = goodFix();
    noTime.hasDateTime = false;
    TEST_ASSERT_FALSE(shouldLog(noTime, false, policy));

    FixQuality noError = goodFix();
    noError.hasErrorEstimate = false;
    TEST_ASSERT_FALSE(shouldLog(noError, false, policy));

    FixQuality inaccurate = goodFix();
    inaccurate.latitudeErrorM = 30.0f;
    TEST_ASSERT_FALSE(shouldLog(inaccurate, false, policy));
}

static void test_should_log_respects_movement_requirement() {
    const LoggingPolicy policy{15.0f, true};
    TEST_ASSERT_FALSE(shouldLog(goodFix(), false, policy));
    TEST_ASSERT_TRUE(shouldLog(goodFix(), true, policy));
}

static void test_csv_line_is_iso8601_utc() {
    const TrackPoint p{50.2538, -61.0345, 5.26f, 123.0f, {2026, 11, 1, 14, 3, 7}};
    char line[96];
    const size_t n = formatCsvLine(line, sizeof line, p);
    TEST_ASSERT_EQUAL_STRING("2026-11-01T14:03:07Z,50.253800,-61.034500,5.3,123.0\n", line);
    TEST_ASSERT_EQUAL(strlen(line), n);
}

static void test_csv_line_reports_too_small_buffer() {
    const TrackPoint p{50.0, 19.0, 0.0f, 0.0f, {2026, 1, 1, 0, 0, 0}};
    char line[10];
    TEST_ASSERT_EQUAL(0, formatCsvLine(line, sizeof line, p));
}

static void test_file_name_is_one_per_utc_day() {
    char name[20];
    formatFileName(name, sizeof name, UtcDateTime{2026, 3, 9, 23, 59, 59});
    TEST_ASSERT_EQUAL_STRING("/2026-03-09.csv", name);
}

static void test_screen_date_time() {
    char text[20];
    formatScreenDateTime(text, sizeof text, UtcDateTime{2026, 11, 1, 9, 5, 0});
    TEST_ASSERT_EQUAL_STRING("09:05 01.11.2026", text);
    TEST_ASSERT_EQUAL(strlen(kNoDateTime), strlen(text));
}

// --- ring buffer ----------------------------------------------------------------

static void test_ring_buffer_keeps_newest_when_full() {
    RingBuffer<int, 3> buffer;
    TEST_ASSERT_TRUE(buffer.push(1));
    TEST_ASSERT_TRUE(buffer.push(2));
    TEST_ASSERT_TRUE(buffer.push(3));
    TEST_ASSERT_TRUE(buffer.full());
    TEST_ASSERT_FALSE(buffer.push(4));  // overwrites 1
    TEST_ASSERT_EQUAL(3, buffer.size());
    TEST_ASSERT_EQUAL(2, buffer.at(0));
    TEST_ASSERT_EQUAL(4, buffer.at(2));
    buffer.clear();
    TEST_ASSERT_TRUE(buffer.empty());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_degrees_minutes_basic);
    RUN_TEST(test_degrees_minutes_rounding_carries_into_degrees);
    RUN_TEST(test_degrees_minutes_rounds_to_nearest_not_up);
    RUN_TEST(test_hemispheres);
    RUN_TEST(test_coordinate_strings_have_fixed_width);
    RUN_TEST(test_speed_rounds_to_tenths_and_clamps);
    RUN_TEST(test_course_wraps_at_360);
    RUN_TEST(test_motion_needs_continuous_movement_to_start);
    RUN_TEST(test_motion_glitch_resets_the_start_timer);
    RUN_TEST(test_motion_stops_after_continuous_absence);
    RUN_TEST(test_motion_survives_millis_wraparound);
    RUN_TEST(test_interval_timer_fires_first_then_per_period);
    RUN_TEST(test_button_reports_one_press_per_push);
    RUN_TEST(test_should_log_requires_position_time_and_accuracy);
    RUN_TEST(test_should_log_respects_movement_requirement);
    RUN_TEST(test_csv_line_is_iso8601_utc);
    RUN_TEST(test_csv_line_reports_too_small_buffer);
    RUN_TEST(test_file_name_is_one_per_utc_day);
    RUN_TEST(test_screen_date_time);
    RUN_TEST(test_ring_buffer_keeps_newest_when_full);
    return UNITY_END();
}
