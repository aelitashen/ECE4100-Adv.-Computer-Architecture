#include "MOESIF_protocol.h"
#include "../sim/mreq.h"
#include "../sim/sim.h"
#include "../sim/hash_table.h"

extern Simulator *Sim;

/*************************
 * Constructor/Destructor.
 *************************/
MOESIF_protocol::MOESIF_protocol (Hash_table *my_table, Hash_entry *my_entry)
    : Protocol (my_table, my_entry)
{
	//initialize lines to not have the data yet 
	this->state = MOESIF_CACHE_I;
}

MOESIF_protocol::~MOESIF_protocol ()
{    
}

void MOESIF_protocol::dump (void)
{
    const char *block_states[12] = {"X","I","S","E","O","M","F", "IM", "IS", "SM", "OM"};
    fprintf (stderr, "MOESIF_protocol - state: %s\n", block_states[state]);
}

void MOESIF_protocol::process_cache_request (Mreq *request)
{
	switch (state) {
	case MOESIF_CACHE_M: do_cache_M(request); break;
	case MOESIF_CACHE_O: do_cache_O(request); break;
	case MOESIF_CACHE_E: do_cache_E(request); break;
	case MOESIF_CACHE_S: do_cache_S(request); break;
	case MOESIF_CACHE_I: do_cache_I(request); break;
	case MOESIF_CACHE_F: do_cache_F(request); break;
	case MOESIF_CACHE_IM: do_cache_IM(request); break;
	case MOESIF_CACHE_IS: do_cache_IS(request); break;
	case MOESIF_CACHE_SM: do_cache_SM(request); break;
	case MOESIF_CACHE_OM: do_cache_OM(request); break;
    default:
        fatal_error ("Invalid Cache State for MOESIF Protocol\n");
    }
}

void MOESIF_protocol::process_snoop_request (Mreq *request)
{
	switch (state) {
	case MOESIF_CACHE_M: do_snoop_M(request); break;
	case MOESIF_CACHE_O: do_snoop_O(request); break;
	case MOESIF_CACHE_E: do_snoop_E(request); break;
	case MOESIF_CACHE_S: do_snoop_S(request); break;
	case MOESIF_CACHE_I: do_snoop_I(request); break;
	case MOESIF_CACHE_F: do_snoop_F(request); break;
	case MOESIF_CACHE_IM: do_snoop_IM(request); break;
	case MOESIF_CACHE_IS: do_snoop_IS(request); break;
	case MOESIF_CACHE_SM: do_snoop_SM(request); break;
	case MOESIF_CACHE_OM: do_snoop_OM(request); break;
    default:
    	fatal_error ("Invalid Cache State for MOESIF Protocol\n");
    }
}

inline void MOESIF_protocol::do_cache_F (Mreq *request)
{
	//forwarder state
	switch (request->msg) {
		case LOAD:
			//we have read access and data so send data to processor
			send_DATA_to_proc(request->addr);
			break;
		case STORE:
			//no write access send GETM on bus, transition to M and wait for data
			state = MOESIF_CACHE_OM;
			send_GETM(request->addr);
			Sim->cache_misses++;
			break;
		default:
			request->print_msg(my_table->moduleID, "ERROR");
			fatal_error("Client: I state shouldn't see this message\n");

	}
}

inline void MOESIF_protocol::do_cache_I (Mreq *request)
{
	//invalid state
	switch (request->msg) {
	case LOAD:
		//send GETS on the bus
		send_GETS(request->addr);
		//transition to transient state and wait for data
		state = MOESIF_CACHE_IS;
		Sim->cache_misses++;
		break;
	case STORE:
		//send GETM on bus and wait for data
		state = MOESIF_CACHE_IM;
		send_GETM(request->addr);
		Sim->cache_misses++;
		break;
	default:
		request->print_msg(my_table->moduleID, "ERROR");
		fatal_error("Client: I state shouldn't see this message\n");
	}
}

inline void MOESIF_protocol::do_cache_S (Mreq *request)
{
	//shared state
	switch (request->msg) {
	case LOAD:
		//we have the data and read access, so send it to the processor
		send_DATA_to_proc(request->addr);
		break;
	case STORE:
		//no write access send GETM on bus and wait for data
		state = MOESIF_CACHE_SM;
		send_GETM(request->addr);
		Sim->cache_misses++;
		break;
	default:
		request->print_msg(my_table->moduleID, "ERROR");
		fatal_error("Client: S state shouldn't see this message\n");
	}
}

inline void MOESIF_protocol::do_cache_E (Mreq *request)
{
	//exclusive state
	switch (request->msg) {
	case LOAD:
		//we have data and read access, send data to processor
		send_DATA_to_proc(request->addr);
		break;
	case STORE:
		//no others have the data, so send data to proc and silently upgrade to M state
		Sim->silent_upgrades++;
		send_DATA_to_proc(request->addr);
		state = MOESIF_CACHE_M;
		break;
	default:
		request->print_msg(my_table->moduleID, "ERROR");
		fatal_error("Client: E state shouldn't see this message\n");
	}
}

inline void MOESIF_protocol::do_cache_O (Mreq *request)
{
	//Owned state
	switch (request->msg) {
	case LOAD:
		//we have the data and read access, so send it to the processor
		send_DATA_to_proc(request->addr);
		break;
	case STORE:
		//no write access send GETM on bus, transition to M and wait for data
		state = MOESIF_CACHE_OM;
		send_GETM(request->addr);
		Sim->cache_misses++;
		break;
	default:
		request->print_msg(my_table->moduleID, "ERROR");
		fatal_error("Client: S state shouldn't see this message\n");
	}
}

inline void MOESIF_protocol::do_cache_M (Mreq *request)
{
	//modified state
	switch (request->msg) {
	case LOAD:
		//we have read access and data, send it to proc
		send_DATA_to_proc(request->addr);
		break;
	case STORE:
		//we have write access and data, send it to proc
		send_DATA_to_proc(request->addr);
		break;
	default:
		request->print_msg(my_table->moduleID, "ERROR");
		fatal_error("Client: M state shouldn't see this message\n");
	}
}

inline void MOESIF_protocol::do_cache_IM(Mreq *request)
{
	switch (request->msg) {
	case LOAD:
	case STORE:
		request->print_msg(my_table->moduleID, "ERROR");
		fatal_error("Should only have one outstanding request per processor!");
	default:
		request->print_msg(my_table->moduleID, "ERROR");
		fatal_error("Client: IM state shouldn't see this message\n");
	}
}

inline void MOESIF_protocol::do_cache_SM(Mreq *request)
{
	do_cache_IM(request);
}

inline void MOESIF_protocol::do_cache_OM(Mreq *request)
{
	do_cache_IM(request);
}

inline void MOESIF_protocol::do_cache_IS(Mreq *request)
{
	do_cache_IM(request);

}

inline void MOESIF_protocol::do_snoop_F (Mreq *request)
{
	do_snoop_O(request);
}

inline void MOESIF_protocol::do_snoop_I (Mreq *request)
{
	//invalid state
	switch (request->msg) {
	case GETS:
		break;
	case GETM:
		break;
	case DATA:
		//don't have the data, cannot downgrade further, do nothing
		break;
	default:
		request->print_msg(my_table->moduleID, "ERROR");
		fatal_error("Client: I state shouldn't see this message\n");
	}
}

inline void MOESIF_protocol::do_snoop_S (Mreq *request)
{
	//shared state
	switch (request->msg) {
	case GETS:
	case GETM:
		//set shared line to show data is being shared on the bus
		set_shared_line();
		//if the request was a GETM, need to downgrade to I since the calling block will go to M
		if (request->msg == GETM)
			state = MOESIF_CACHE_I;
		break;
	case DATA:
		break;
	default:
		request->print_msg(my_table->moduleID, "ERROR");
		fatal_error("Client: I state shouldn't see this message\n");
	}
}

inline void MOESIF_protocol::do_snoop_E (Mreq *request)
{
	//exclusive state
	switch (request->msg) {
	case GETS:
	case GETM:
		//on snooping a GETS transition to F state since another cache will have the block
		state = MOESIF_CACHE_F;
		if (request->msg == GETM)
			//change to I on a GETM since another block will be going to M 
			state = MOESIF_CACHE_I;
		//set shared line to show data is being shared on the bus
		set_shared_line();
		//we have the data so send the data on the bus on a GET request
		send_DATA_on_bus(request->addr, request->src_mid);
		break;
	case DATA:
		break;
	default:
		request->print_msg(my_table->moduleID, "ERROR");
		fatal_error("Client: I state shouldn't see this message\n");
	}
}

inline void MOESIF_protocol::do_snoop_O(Mreq *request)
{
	//Owned state
	switch (request->msg) {
	case GETS:
		//on a GETS, stay in O since there can be other blocks in S
	case GETM:
		if (request->msg == GETM)
			//change to I on a GETM since another block will be going to M
			state = MOESIF_CACHE_I;
		//we have the data so send the data on the bus on a GET request
		send_DATA_on_bus(request->addr, request->src_mid);
		set_shared_line();
		break;
	case DATA:
		break;
	default:
		request->print_msg(my_table->moduleID, "ERROR");
		fatal_error("Client: I state shouldn't see this message\n");
	}
}

inline void MOESIF_protocol::do_snoop_M (Mreq *request)
{
	//Modified State
	switch (request->msg) {
	case GETS:
	case GETM:
		//change to O on a GETS since another block will be going to S
		state = MOESIF_CACHE_O;
		if (request->msg == GETM)
			//change to I on a GETM since another block will be going to M 
			state = MOESIF_CACHE_I;
		//set shared line to show data is being shared on the bus
		set_shared_line();
		//we have the data so send the data on the bus on a GET request
		send_DATA_on_bus(request->addr, request->src_mid);
		break;
	case DATA:
		break;
	default:
		request->print_msg(my_table->moduleID, "ERROR");
		fatal_error("Client: I state shouldn't see this message\n");
	}
}

inline void MOESIF_protocol::do_snoop_SM(Mreq *request)
{
	//transient state
	switch (request->msg) {
	case GETS:
		set_shared_line();
		break;
	case GETM:
		set_shared_line();
		break;
	case DATA:
		/** SM state meant that the block had sent the GET and was waiting on DATA.
		* Now that Data is received we can send the DATA to the processor and finish
		* the transition to M.
		*/
		send_DATA_to_proc(request->addr);
		state = MOESIF_CACHE_M;
		break;
	default:
		request->print_msg(my_table->moduleID, "ERROR");
		fatal_error("Client: I state shouldn't see this message\n");

	}
}

inline void MOESIF_protocol::do_snoop_IM(Mreq *request)
{
	//transient state
	switch (request->msg) {
	case GETS:
		break;
	case GETM:
		break;
	case DATA:
		/** IM state meant that the block had sent the GET and was waiting on DATA.
		* Now that Data is received we can send the DATA to the processor and finish
		* the transition to M.
		*/
		send_DATA_to_proc(request->addr);
		state = MOESIF_CACHE_M;
		break;
	default:
		request->print_msg(my_table->moduleID, "ERROR");
		fatal_error("Client: I state shouldn't see this message\n");

	}
}


inline void MOESIF_protocol::do_snoop_OM(Mreq *request)
{
	//transient state
	switch (request->msg) {
	case GETS:
		//on a GETS send data to bus since we have the data
		send_DATA_on_bus(request->addr, request->src_mid);
		set_shared_line();
		break;
	case GETM:
		//on a GETM send data to bus since we have the data
		state = MOESIF_CACHE_SM;
		send_DATA_on_bus(request->addr, request->src_mid);
		set_shared_line();
		break;
	case DATA:
		//once we get the data, we can now transition to M
		send_DATA_to_proc(request->addr);
		state = MOESIF_CACHE_M;
		break;
	default:
		request->print_msg(my_table->moduleID, "ERROR");
		fatal_error("Client: I state shouldn't see this message\n");

	}
}

inline void MOESIF_protocol::do_snoop_IS(Mreq *request)
{
	switch (request->msg) {
	case GETS:
		break;
	case GETM:
		break;
	case DATA:
		/** IS state meant that the block had sent the GET and was waiting on DATA.
		* Now that Data is received we can send the DATA to the processor and finish
		* the transition to either E or S depending on whether other processors have the block or not.
		*/
		send_DATA_to_proc(request->addr);
		state = MOESIF_CACHE_E;
		if (get_shared_line()) {
			state = MOESIF_CACHE_S;
		}
		break;
	default:
		request->print_msg(my_table->moduleID, "ERROR");
		fatal_error("Client: I state shouldn't see this message\n");

	}
}



