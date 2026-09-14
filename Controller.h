#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "ToasterTypes.h"

class Controller
{
public:
	//Controller(MessageHandler* messageHandler, ApiManager* apiManager);
	//void handleDefault(const InternalMessage& msg);
	void handleAudio(const IQData& iqData);




private:

};

#endif