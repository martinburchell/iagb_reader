#!/usr/bin/python

import os
from glob import iglob

disc_a = os.path.join('/mnt', 'iagb_a')
disc_b = os.path.join('/mnt', 'iagb_b')

chp_dirs = (os.path.join(disc_a, 'Pictures', 'SCALES', '50K'),
            os.path.join(disc_a, 'Pictures', 'Towns', 'LRG'),
            os.path.join(disc_b, 'Pictures', 'SCALES', '25k'))

ppm_dir = os.path.join('..', '..', 'ppm')
chp2ppm_bin = os.path.join('..', 'bin', 'chp2ppm')

for chp_dir in chp_dirs:
    print chp_dir
    for chp_path in iglob(os.path.join(chp_dir, '*.chp')):
        print chp_path
        (head, tail) = os.path.split(chp_path)
        (map_name, ext) = os.path.splitext(tail)
        ppm_name = map_name + '.ppm'
        ppm_file = os.path.join(ppm_dir, ppm_name)
        command = "%s %s %s" % (chp2ppm_bin, chp_path, ppm_file)
        os.system(command)
