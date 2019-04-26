/*
       Copyright (c) 2019 Kody Kinzie, Brandon Paiz for Leadership In Technology & RETIA.IO
       This software is licensed under the MIT License. See the license file for details.
       TUNNEL SNAKES RULE!

       This example is based on the simpleCLI example for the ESP8266, Copyright (c) 2019 Stefan Kremser

       The Chicken Man Game is a multi-player Wi-Fi cracking game. You can use 1, 2, or 3 players (or teams).
       The esp8266 checks the difficulty level and creates a Wi-Fi network using a password from that difficulty list
       Once the player cracks the password and logs in to the Wi-Fi network, they can set the LED to their team color.
       Setting the LED causes the esp8266 to close the connection and create a new Wi-Fi network with a harder password.
       Players can then attempt to steal the flag by hacking the harder password and setting the LED to their team color.
       Currently supports easy, medium, and hard mode. Password lists can be expanded to avoid collisions.
 */

// ========== Includes ========== //

// [Libraries]
 #include <SimpleCLI.h>

// [Local files]
#include "Bird.h"
#include "Man.h"
#include "LED.h"
#include "Web.h"
#include "EEPROMHelper.h"

#include "config.h"
#include "hardware.h"
#include "types.h"

// ========== Global Variables ========== //

Bird bird; // Chicken game instance
Man  man;  // Chicken man
LED  led;  // LED
Web  web;  // Web interface

// Command Line Interface
SimpleCLI cli;
Command   cmdFlag;
Command   cmdPoints;
Command   cmdReset;

int pointBlinkCounter = 0;
GAME_TYPE type        = NO_TYPE;

// ========== Global Functions ========== //

// CLI handler that is called via serial and the web interface
String handleCLI(String input) {
    // Parse input
    cli.parse(input);

    // Check for commands
    if (cli.available()) {
        // Get the command that was entered/parsed successfully
        Command cmd = cli.getCommand();

        // Flag
        if ((cmd == cmdFlag) && (type == CHICKEN)) {
            // Get the team color that was entered
            String team = cmd.getArgument("team").getValue();

            // Set flags and return a reply string
            if (team.equalsIgnoreCase("blue")) {
                bird.setFlag(BLUE);
                return "Why so blue?";
            } else if (team.equalsIgnoreCase("red")) {
                bird.setFlag(RED);
                return "Seeing red?";
            } else if (team.equalsIgnoreCase("green")) {
                bird.setFlag(GREEN);
                return "The others must be green with envy!";
            } else {
                return "Wrong team, mate!";
            }
        }

        // Points
        else if ((cmd == cmdPoints) && (type == CHICKEN)) {
            pointBlinkCounter = 4;
            return bird.getPointsString(true);
        }

        // Reset
        else if ((cmd == cmdReset) && (type == CHICKEN)) {
            if (bird.resetGame(cmd.getArgument("pswd").getValue())) {
                return "Resetted game stats!";
            };
        }
    }

    // Check for errors
    if (cli.errored()) {
        return cli.getError().toString();
    }

    // Return empty string if everything else fails
    return String();
}

// ========== Setup ========== //
void setup() {
    // Start Serial
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println("Starting");

    // Random seed
    randomSeed(os_random());

    // If resetted 3 times in a row
    if (!EEPROMHelper::checkBootNum(EEPROM_SIZE, EEPROM_BOOT_ADDR)) {
        // Erase (overwrite) old stats
        game_stats emptyStats;
        EEPROMHelper::saveObject(EEPROM_SIZE, EEPROM_STATS_ADDR, emptyStats);
    }

    // Setup LED(s)
    led.begin();

    // Setup Switch
    pinMode(SWITCH_PIN, INPUT_PULLUP);

    // CLI
    cmdFlag = cli.addCommand("flag,color,led");
    cmdFlag.addPositionalArgument("t/eam", "green");

    cmdPoints = cli.addCommand("points");

    cmdReset = cli.addCommand("reset");
    cmdReset.addArgument("p/assword,pswd");

    if (digitalRead(SWITCH_PIN) == LOW) {
        Serial.println("Mode: Chicken Man");

        type = CHICKEN_MAN;

        // Make sure it's in station mode
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();

        man.begin();
    } else {
        Serial.println("Mode: Chicken");

        type = CHICKEN;

        // Setup Game Access Point
        bird.begin();

        // If something went wrong, blink like crazy for 5s and then restart
        if (bird.errored()) {
            unsigned long startTime = millis();

            while (millis() - startTime >= 5000) {
                led.blink(10, NO_TEAM);
            }

            ESP.restart();
        }

        // Start web interface
        web.begin();
    }

    // Set boot counter back to 1
    EEPROMHelper::resetBootNum(EEPROM_SIZE, EEPROM_BOOT_ADDR);
}

// ========== Loop ========== //
void loop() {
    if (type == CHICKEN) {
        // Update game (access point, server, ...)
        bird.update();

        // Blink LED(s) when eggs are gathered
        if (pointBlinkCounter) {
            pointBlinkCounter = led.blink(8, bird.getFlag(), pointBlinkCounter);
        }
        // Otherwise set the current flag color
        else {
            led.setColor(bird.getFlag());
        }

        // Serial CLI
        if (Serial.available()) {
            String input = Serial.readStringUntil(SERIAL_DELIMITER);

            // Echo the input on the serial interface
            Serial.print("# ");
            Serial.println(input);

            Serial.println(handleCLI(input));
        }

        // Web server
        web.update();
    } else {
        man.update();
        led.setColor(man.getFlag());

        Serial.println("Going to sleep for 10s...zZzZ");
        delay(10000);
    }
}