#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "ToasterTypes.h"

#include <cstdint>
#include <complex>
#include <functional>
#include <unordered_map>
#include <vector>

class Dispatcher
{
public:
    //using IQData = IQData;
    using Handler = std::function<void(const IQData&)>;

    // Type would be an enum representing the component i want to send the iq data too
    void registerHandler(int type, Handler handler);
    //void registerHandler(Handler handler);
    void dispatch(const uint8_t* buf, uint32_t len);
    void dispatch(const IQData& iqData);

private:
    std::unordered_map<int, Handler> handlers_;
};


//#include <functional>
//
//class Dispatcher
//{
//public:
//    using Handler = std::function<void(const InternalMessage&)>;
//    void registerHandler(MessageType type, Handler handler);
//
//	// TODO: only need one of these. find out which one is needed and remove the others
//    void dispatch(const InternalMessage& msg);
//    void dispatch(InternalMessage& msg);
//private:
//    std::unordered_map<MessageType, Handler> handlers_;
//};

#endif