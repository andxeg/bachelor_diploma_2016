#include "defs.h"
#include "interface/snapshot.h"
#include "trans/mixer.h"

#include <QString>

#include <stdio.h>

int main(int argc, char ** argv)
{
    if ( argc != 3 )
    {
        printf("Usage: %s <initial configuration> <final configuration>\n", *argv);
        return 1;
    }

   Snapshot from, to;
   from.read(argv[1]);
   to.read(argv[2]);

   Mixer mixer;
   mixer.read(from, to);
   mixer.print();

   return 0;
}
