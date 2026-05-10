/**
 * @file test_simulation.cpp
 * @brief Basic simulation tests
 */

#include "core/simulation_engine.h"
#include "core/elf_loader_wrapper.h"
#include "peripherals/gpio_controller.h"
#include "utils/logger.h"

#include <QCoreApplication>
#include <QtTest>

#include <cstdio>

class SimulationTests : public QObject {
    Q_OBJECT

private slots:
    void testMemoryModel();
    void testGPIO();
    void testElfLoader();
    void testSimulationEngine();
    void testPeripherals();
};

void SimulationTests::testMemoryModel() {
    LOG_INFO("Running testMemoryModel");

    auto memory = std::make_unique<MemoryModel>();

    // Test basic read/write
    memory->write_word(0x3FFB0000, 0x12345678);
    uint32_t val = memory->read_word(0x3FFB0000);
    QCOMPARE(val, 0x12345678u);

    // Test invalid address
    bool exception_thrown = false;
    try {
        memory->read_word(0xFFFFFFFF);
    } catch (const MemoryAccessException&) {
        exception_thrown = true;
    }
    QVERIFY(exception_thrown);

    LOG_INFO("testMemoryModel PASSED");
}

void SimulationTests::testGPIO() {
    LOG_INFO("Running testGPIO");

    auto memory = std::make_unique<MemoryModel>();
    auto gpio = std::make_unique<GPIOController>(memory.get());

    QVERIFY(gpio->initialize());

    // Configure and set output
    gpio->configurePin(2, GPIOMode::OUTPUT);
    gpio->setLevel(2, GPIOLevel::HIGH);

    auto level = gpio->getLevel(2);
    QCOMPARE(level, GPIOLevel::HIGH);

    // Input with pull-up
    gpio->configurePin(0, GPIOMode::INPUT_PULLUP);
    auto input_level = gpio->getLevel(0);
    QCOMPARE(input_level, GPIOLevel::HIGH);  // Pull-up makes input HIGH

    LOG_INFO("testGPIO PASSED");
}

void SimulationTests::testElfLoader() {
    LOG_INFO("Running testElfLoader");

    ElfLoaderWrapper loader;

    // Test loading invalid file
    bool result = loader.load("nonexistent.elf");
    QVERIFY(!result);
    QVERIFY(loader.lastError().find("Failed") != std::string::npos);

    LOG_INFO("testElfLoader PASSED (negative test)");
}

void SimulationTests::testSimulationEngine() {
    LOG_INFO("Running testSimulationEngine");

    auto engine = std::make_unique<SimulationEngine>();
    QVERIFY(engine->initialize());

    QCOMPARE(engine->state(), SimulationState::STOPPED);

    engine->start();
    QCOMPARE(engine->state(), SimulationState::RUNNING);

    engine->pause();
    QCOMPARE(engine->state(), SimulationState::PAUSED);

    engine->resume();
    QCOMPARE(engine->state(), SimulationState::RUNNING);

    engine->stop();
    QCOMPARE(engine->state(), SimulationState::STOPPED);

    LOG_INFO("testSimulationEngine PASSED");
}

void SimulationTests::testPeripherals() {
    LOG_INFO("Running testPeripherals");

    // Test ADC
    {
        auto memory = std::make_unique<MemoryModel>();
        auto adc = std::make_unique<ADCController>(memory.get());
        adc->initialize();

        adc->configureChannel(0, ADCAttenuation::ATTEN_0DB, 36);
        adc->setExternalVoltage(36, 1.65f);
        uint16_t raw = adc->getADCValue(0);
        QVERIFY(raw > 0 && raw < 4096);
    }

    // Test UART
    {
        auto memory = std::make_unique<MemoryModel>();
        auto uart = std::make_unique<UARTController>(0, memory.get());
        uart->initialize();
        uart->configure(115200);

        uint8_t data = 0x55;
        QCOMPARE(uart->writeByte(data), ESP_OK);
        QVERIFY(uart->txFIFOLevel() > 0);
    }

    LOG_INFO("testPeripherals PASSED");
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // Initialize logging
    Logger::instance().setLevel(LogLevel::DEBUG);
    Logger::instance().addConsoleSink();

    LOG_INFO("=== Starting Simulation Tests ===");

    SimulationTests tests;
    int result = QTest::qExec(&tests, argc, argv);

    LOG_INFO("=== Tests completed with code {} ===", result);
    return result;
}

#include "test_simulation.moc"
