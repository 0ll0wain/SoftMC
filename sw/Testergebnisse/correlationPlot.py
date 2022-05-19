from cProfile import label
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
from scipy.stats.stats import pearsonr
import math

plt.style.use('bmh')
fig = plt.figure()
ax = fig.add_subplot()

filesA = [
"/home/kim/Projects/SoftMC/sw/Testergebnisse/Data/MicronLong24°C/MicronLong24x0.csv",
"/home/kim/Projects/SoftMC/sw/Testergebnisse/Data/MicronLong24°C/MicronLong24xff.csv",
"/home/kim/Projects/SoftMC/sw/Testergebnisse/Data/MicronLong40°C/MicronLong40x0.csv",
"/home/kim/Projects/SoftMC/sw/Testergebnisse/Data/MicronLong40°C/MicronLong40xff.csv",
]
filesB = ["/home/kim/Projects/SoftMC/sw/Testergebnisse/Data/SamsungLong24°C/SamsungLong24x0.csv",
"/home/kim/Projects/SoftMC/sw/Testergebnisse/Data/SamsungLong24°C/SamsungLong24xff.csv",
"/home/kim/Projects/SoftMC/sw/Testergebnisse/Data/SamsungLong40°C/SamsungLong40x0.csv",
"/home/kim/Projects/SoftMC/sw/Testergebnisse/Data/SamsungLong40°C/SamsungLong40xff.csv"
]
labelsA = ["24°C, 0x00", "24°C, 0xff", "40°C, 0x00", "40°C, 0xff"]
labelsB = ["24°C, 0x00", "24°C, 0xff", "40°C, 0x00", "40°C, 0xff"]

ax.set_ylim(ymin=-1, ymax=1)
ax.set_title("Correlation between retention time and errors, Chip B")

for i in range(0,4):

    file = filesB[i]
    l = labelsB[i]

    correlations = []
    df = pd.read_csv(file)
    tRCDvalues = df["tRCD"].unique()
    tRCDvalues.sort()
    for tRCD in tRCDvalues:
        data = df[df.tRCD == tRCD]
        corr = data["FehlerProzent"].corr(data["retention"])
        if math.isnan(corr): corr = 0
        correlations.append(corr )
    r = pearsonr(tRCDvalues, correlations)
    print(l + ", total correalation coefficient = " + str(r[0]) + ", p-value = " + str(r[1]))
    ax.plot(tRCDvalues, correlations, label=l)
    


ax.set_xlabel("tRCD in ns")
ax.set_ylabel("Correlation")
ax.legend()

plt.show()
    





