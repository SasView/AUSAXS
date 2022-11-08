import matplotlib.pyplot as plt
import numpy as np
import sys
import os

from plot_helper import *

# handle command line arguments
if len(sys.argv) < 2:
    print("Usage: python plot.py <filename>")
    sys.exit(1)

if (sys.argv[1] == "-h") or (sys.argv[1] == "--help"):
    print("Plotting tool for the .plot files from AUSAXS.")
    sys.exit(1)

params = {'legend.fontsize': 'x-large',
          'figure.figsize': (15, 5),
         'axes.labelsize': 'x-large',
         'axes.titlesize':'x-large',
         'xtick.labelsize':'x-large',
         'ytick.labelsize':'x-large'}
plt.rcParams.update(params)

folder = sys.argv[1]
for currentpath, folders, files in os.walk(folder):
    for file in files:
        if file.endswith(".plot"):
            print("Plotting file: " + file)
            plot_file(os.path.join(currentpath, file))