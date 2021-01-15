#pragma once
// openFrameworks
#include "ofTypes.h"
//#include "ofBaseSoundStream.h"
#include "ofJson.h"
#include "ofPixels.h"
#include "ofRectangle.h"
#include "ofSoundBaseTypes.h"
#include "ofVideoBaseTypes.h"
#include <mutex>

#if defined( TARGET_OSX )
#include <thread>
#endif

namespace ofxFFmpeg {

using Clock     = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;
using Seconds   = std::chrono::duration<float>;

/**
 * LockFreeQueue is taken from here: https://github.com/timscaffidi/ofxVideoRecorder/blob/master/src/ofxVideoRecorder.h#L9
 */
template <typename T>
struct LockFreeQueue
{

	LockFreeQueue()
	{
		// Add one to validate the iterators
		m_List.push_back( T() );
		m_HeadIt = m_List.begin();
		m_TailIt = m_List.end();
	}

	void produce( const T& t )
	{
		m_List.push_back( t );
		m_TailIt = m_List.end();
		m_List.erase( m_List.begin(), m_HeadIt );
	}

	bool consume( T& t )
	{
		typename TList::iterator nextIt = m_HeadIt;
		++nextIt;
		if ( nextIt != m_TailIt ) {
			m_HeadIt = nextIt;
			t        = *m_HeadIt;
			return true;
		}

		return false;
	}

	int size() const
	{
		return std::distance( m_HeadIt, m_TailIt ) - 1;
	}

	//typename std::list<T>::iterator getHead() const
	//{
	//	return m_HeadIt;
	//}

	//typename std::list<T>::iterator getTail() const
	//{
	//	return m_TailIt;
	//}

private:
	using TList = std::list<T>;
	TList m_List;
	typename TList::iterator m_HeadIt, m_TailIt;

};
}  // namespace ofxFFmpeg