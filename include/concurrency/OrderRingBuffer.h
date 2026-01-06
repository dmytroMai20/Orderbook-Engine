#pragma once

#include "SPSCRingBuffer.h"
#include "EngineEvent.h"

using OrderRingBuffer = SPSCQueue<EngineEvent, 16384>;