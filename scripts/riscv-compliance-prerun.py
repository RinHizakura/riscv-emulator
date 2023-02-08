#!/usr/bin/env python3

import os
import configparser

riscof_test_path = os.getcwd() + "/riscof-test"
config_path = riscof_test_path + "/template.ini"

config = configparser.ConfigParser()
config.read(config_path)

if config.has_option("RISCOF", "riscof-test-path") == False :
    config.set("RISCOF", "riscof-test-path", riscof_test_path)

if config.has_option("riscv_emulator", "DUTPluginPath") == False :
    config.set("riscv_emulator", "DUTPluginPath", config["RISCOF"]["DUTPluginPath"])
if config.has_option("riscv_emulator", "PWD") == False :
    config.set("riscv_emulator", "PWD", os.getcwd())

if config.has_option("sail_cSim", "ReferencePluginPath") == False :
    config.set("sail_cSim", "ReferencePluginPath", config["RISCOF"]["ReferencePluginPath"])

with open(riscof_test_path + "/config.ini", 'w') as configfile:
    config.write(configfile)
