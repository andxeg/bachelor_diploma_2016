#include "mixer.h"
#include "task.h"

#include "network.h"

#include "interface/snapshot.h"

#include <QString>

#include <stdio.h>

Mixer::Mixer() {}

bool Mixer::read(const Snapshot & from, const Snapshot & to) {
    network = from.getNetwork(); 

    Snapshot::Assignments f = from.parseReverseAssignments();
    Snapshot::Assignments t = to.parseReverseAssignments();
    foreach(QString tenant, f.keys()) {
        QMap<QString, QString> & ft = f[tenant];
        QMap<QString, QString> & tt = t[tenant];
        foreach(QString element, ft.keys()) {
            if ( ft[element] == tt[element])
                continue;

            Task * task = new Task(from.getTenantElement(tenant, element), from.getNetworkElement(tt[element]));
            tasks.push_back(task);
        }
    }

    return true;
}

void Mixer::print() const
{
    fprintf(stderr, "[MIXER] generated %i tasks:\n", tasks.size());
    for(std::list<Task *>::const_iterator i = tasks.begin(); i != tasks.end(); i++)
    {
        Task * task = *i;
        fprintf(stderr, "[MIXER]\t%p:%p->%p\n", task->getTarget(), task->getSource(), task->getDestination());
    }

}
