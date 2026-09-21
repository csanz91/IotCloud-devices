#pragma once
inline unsigned long os_random() { static unsigned long next = 123; return ++next; }
