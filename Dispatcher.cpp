#include "Dispatcher.h"
#include <iostream>
#include <cstdint>


void Dispatcher::registerHandler(int type, Handler handler)
{
    handlers_[type] = std::move(handler);
}

/*
void Dispatcher::dispatch(const uint8_t* buf, uint32_t len) 
{
    // std::cout << "Dispatcher::dispatch()" << std::endl;
    // Decide which handler should receive the IQ data.

    //auto it = handlers_.find(msg.header_.messageType_);
    auto it = handlers_.find(1);
    if (it != handlers_.end())
    {
        it->second(buf, len);
    }
    else
    {
        std::cout << "No handlers found." << std::endl; 
    }
}*/

void Dispatcher::dispatch(const IQData& iqData)
{
    // std::cout << "Dispatcher::dispatch()" << std::endl;
    // Decide which handler should receive the IQ data.

    //auto it = handlers_.find(msg.header_.messageType_);
    auto it = handlers_.find(0);
    if (it != handlers_.end())
    {
        it->second(iqData);
    }
    else
    {
        std::cout << "No handlers found." << std::endl; 
    }
}

// void Dispatcher::registerHandler(MessageType type, Handler handler)
// {
// 	handlers_[type] = std::move(handler);
// }
// 
// void Dispatcher::dispatch(const InternalMessage& msg)
// {
// 	std::cout << "Dispatcher::dispatch(const InternalMessage& msg)" << std::endl;
// 	auto it = handlers_.find(msg.header_.messageType_);
// 	if (it != handlers_.end())
// 	{
// 		it->second(msg);
// 	}
// 	else
// 	{
// 		std::cout << "No handlers found for the given messageType: " << msg.header_.messageType_ << std::endl;
// 	}
// }
// 
// void Dispatcher::dispatch(InternalMessage& msg)
// {
// 	std::cout << "Dispatcher::dispatch(InternalMessage& msg)" << std::endl;
// 	auto it = handlers_.find(msg.header_.messageType_);
// 	if (it != handlers_.end())
// 	{
// 		it->second(msg);
// 	}
// 	else
// 	{
// 		std::cout << "No handlers found for the given messageType: " << msg.header_.messageType_ << std::endl;
// 	}
// }