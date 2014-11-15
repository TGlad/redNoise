#include "Timer.h"
#include <stdio.h>

#include <windows.h>

//----------------------------------------------------------------------------------------------------------------------
unsigned long GetTimerFrequency()
{
  unsigned long result;
  LARGE_INTEGER frequency;
  QueryPerformanceFrequency(&frequency); 
  result = (unsigned long)frequency.QuadPart;
  return result;
}

//----------------------------------------------------------------------------------------------------------------------
unsigned long GetCurrentTimerValue()
{
  unsigned long result;
  LARGE_INTEGER frequency;
  QueryPerformanceCounter(&frequency); 
  result = (unsigned long)frequency.QuadPart;
  return result;
}

Timer::Timer()
{
  m_frequency = GetTimerFrequency(); 
  m_totalSectionTime = 0;
}

Timer::~Timer()
{
}

void Timer::start()
{
  m_sectionStartTime = GetCurrentTimerValue();
}

TimePeriod Timer::stop()
{
  m_sectionEndTime = GetCurrentTimerValue();
  TimePeriod elapsedTime = getElapsedTime(m_sectionStartTime, m_sectionEndTime);
  m_totalSectionTime += elapsedTime;
  return elapsedTime;
}

void Timer::reset()
{
  m_sectionEndTime = 0;
  m_totalSectionTime = 0;
  m_sectionStartTime = 0;
}

TimePeriod Timer::getTotalTime()
{
  return m_totalSectionTime;
}


void Timer::print(const char* outputString)
{
  printf("Total for %s: %f ms\n", outputString, m_totalSectionTime);
}

TimePeriod Timer::getElapsedTime(const unsigned long start, const unsigned long end) const
{
  if (m_frequency == 0)
    return 0.0;
  else
  {
    TimePeriod result = getTime(end - start);
    return result;
  }
}

TimePeriod Timer::getTime(unsigned long elapsed) const
{
  return TimePeriod( (elapsed) * 1000.0 / m_frequency );
}
