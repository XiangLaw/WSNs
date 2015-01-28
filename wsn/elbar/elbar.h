#ifndef _ELBAR_H_
#define _ELBAR_H_

#include <vector>
#include "../geomathhelper/geo_math_helper.h"
#include "../gpsr/gpsr.h"
#include "../gridonline/gridonline.h"

class ElbarGridOnlineAgent;

class ElbarGridOnlineAgent: public GridOnlineAgent {
public:
	ElbarGridOnlineAgent();
	int 	command(int, const char*const*);
	void 	recv(Packet*, Handler*);

protected:
	void routing();
};

#endif
