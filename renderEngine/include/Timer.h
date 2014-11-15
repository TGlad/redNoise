#pragma once
#include "basics.h"

unsigned long GetTimerFrequency();
unsigned long GetCurrentTimerValue();

class Timer
{
public:
  Timer();
  ~Timer();

  void  start();
  TimePeriod stop();
  void  reset();
  TimePeriod getTotalTime();

  void print(const char* outputString);

private:
  TimePeriod getElapsedTime(const unsigned long start, const unsigned long end) const;
  TimePeriod getTime(unsigned long elapsed) const;

  TimePeriod m_totalSectionTime;
  unsigned long m_sectionStartTime; 
  unsigned long m_sectionEndTime;
  unsigned long m_frequency;
};
