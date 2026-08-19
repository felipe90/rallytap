/**
 * Native test suite runner.
 *
 * PlatformIO's [env:native] links every test_*.cpp file in test/ into a
 * single executable, so exactly one main()/setUp()/tearDown() may exist.
 * Each test file exposes a runTests_<name>() entrypoint that registers its
 * cases; this runner drives them all inside one UNITY_BEGIN/END session.
 */
#include <Arduino.h>
#include <unity.h>

extern void runTests_score();
extern void runTests_wifiBackoff();
extern void runTests_buttonHandler();
extern void runTests_protocolFixture();
extern void runTests_mesaLabels();
extern void runTests_networkProfiles();

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char** argv) {
    UNITY_BEGIN();

    runTests_score();
    runTests_wifiBackoff();
    runTests_buttonHandler();
    runTests_protocolFixture();
    runTests_mesaLabels();
    runTests_networkProfiles();

    return UNITY_END();
}
