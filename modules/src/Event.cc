#include "Event.hh"

Event::Event(RawEventPtr raw) :  _raw_event(raw), 
				 _event_data(new EventData) 
{}

Event::~Event() 
{ }

