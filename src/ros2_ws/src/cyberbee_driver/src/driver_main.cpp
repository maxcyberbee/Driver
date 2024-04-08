/*
 * Copyright (c) 2024 CyberBee Ltd.
 * All rights reserved.
 *
 * This file is part of the CyberBee IMX8 C++ Driver.
 * 
 * Description:
 * This code is part of the CyberBee  IMX8 C++ Driver layer, responsible for
 * managing communication between imx8 and other devices sucha s PI-4B.
 * This code meant to be compiled on the client device i.e. PI-4B,or other ubuntu system.
 *
 */

#include <csignal>
#include <iostream>
#include <memory>
#include <thread>  // Include the thread header

#include "../include/cyberbee_driver/data_receiver_ros.hpp"
#include "../include/cyberbee_driver/data_sender.hpp"
#include "../include/cyberbee_driver/serial_comm.hpp"  // Adjust the include path according to your project structure

std::unique_ptr<SerialPort> globalSerialPort;
std::unique_ptr<DataReceiverRos> data_receiver;
std::unique_ptr<DataSender> data_sender;

// Global variables
uint16_t division_rate = 0;
uint8_t baud_rate_index = 4;  // Default baud rate index for 115200
bool testing_baud = false;
int baud_rates[5] = { 9600, 19200, 38400, 57600, 115200 };  // Baud rates list
bool listen_only = false;

enum UserCommand
{
    Test = 0x20,
    SetDivisionRate = 0x22,
    Confirm = 0x23,
    ChangeBaudRate = 0x30
    // Add more commands as needed
};

void signalHandler(int signum)
{
    std::cout << "Interrupt signal (" << signum << ") received.\n";

    // Signal DataReceiver to stop
    if (data_receiver)
    {
        data_receiver->stop();
        rclcpp::shutdown();
    }
    // Terminate program
    exit(signum);
}

void readDataThread(DataReceiverRos* data_receiver)
{
    while (true)
    {
        data_receiver->readData();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main()
{
    // Register signal SIGINT and signal handler
    signal(SIGINT, signalHandler);
    rclcpp::init(0, nullptr);
    auto node = std::make_shared<rclcpp::Node>("cyberbee_driver_node");

    std::string portName = "/dev/ttyUSB0";  // ttyUSBO is the port on PC
    unsigned int baudRate = 115200;         // current baud rate for PC
    char command_input;
    // Create an instance of SerialPort
    globalSerialPort = std::make_unique<SerialPort>(portName, baudRate);

    // Start communication in a separate thread
    globalSerialPort->OpenSerialConnection();

    data_receiver = std::make_unique<DataReceiverRos>(node, globalSerialPort.get());
    data_sender = std::make_unique<DataSender>(globalSerialPort.get());

    // Create a thread for reading data and another for extracting it from buff
    std::thread readThread(&DataReceiverRos::readData, data_receiver.get());
    std::thread processThread(&DataReceiverRos::processData, data_receiver.get());

    // Wait for the threads to finish
    readThread.join();
    processThread.join();
    rclcpp::spin(node);
    return 0;
}