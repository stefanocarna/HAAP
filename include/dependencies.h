#ifndef _DEPENDENCIES_H
#define _DEPENDENCIES_H

extern int idt_patcher_install_entry(unsigned long handler, unsigned vector, unsigned dpl);

#endif /* _DEPENDENCIES_H */