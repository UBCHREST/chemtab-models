import os, argparse
import yaml

# example metadata.yaml:
"""
---
# all paths are assumed to be relative
name: PCDNNV2_decomp-Wax-Orthog+Zmix
rpath: regressor
wpath: weights.csv
mechanism: GRIMech30.yaml
version: 1.0
"""

if __name__=='__main__':
    parser = argparse.ArgumentParser(description = 'makes metadata.yaml for Matt')
    parser.add_argument('model_folder', type=str, help='model folder to create metadata.yaml file for')
    parser.add_argument('mech_filename', type=str, help='relative path to mechanism file to include inside the metadata.yaml file')
    args = parser.parse_args()

    # NOTE: as of right now the code ignores the rpath & wpath variables
    cfg = {'rpath': 'regressor', 'wpath': 'weights.csv', 'version': 1.0}
    cfg['name']=os.path.basename(args.model_folder)
    cfg['mechanism']=os.path.basename(args.mech_filename)
    
    os.system(f'cp {args.mech_filename} {args.model_folder}/.')
    with open(f'{args.model_folder}/metadata.yaml', 'w') as f:
        yaml.dump(cfg, f)
    
