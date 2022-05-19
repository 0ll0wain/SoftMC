#%%
import matplotlib.pyplot as plt
from matplotlib.ticker import FormatStrFormatter
from matplotlib.pyplot import cm
import numpy as np
import pandas as pd

plt.style.use('bmh')
fig = plt.figure()
ax = fig.add_subplot()

#df = pd.read_csv("/home/kim/Projects/SoftMC/sw/Testergebnisse/Data/MicronLong24°C/MicronLong24x0.csv")
#df = pd.read_csv("/home/kim/Projects/SoftMC/sw/Testergebnisse/Data/MicronLong24°C/MicronLong24xff.csv")
#df = pd.read_csv("/home/kim/Projects/SoftMC/sw/Testergebnisse/Data/MicronLong40°C/MicronLong40x0.csv")
#df = pd.read_csv("/home/kim/Projects/SoftMC/sw/Testergebnisse/Data/MicronLong40°C/MicronLong40xff.csv")
#df = pd.read_csv("/home/kim/Projects/SoftMC/sw/Testergebnisse/Data/SamsungLong24°C/SamsungLong24x0.csv")
#df = pd.read_csv("/home/kim/Projects/SoftMC/sw/Testergebnisse/Data/SamsungLong24°C/SamsungLong24xff.csv")
#df = pd.read_csv("/home/kim/Projects/SoftMC/sw/Testergebnisse/Data/SamsungLong40°C/SamsungLong40x0.csv")
df = pd.read_csv("/home/kim/Projects/SoftMC/sw/Testergebnisse/Data/SamsungLong40°C/SamsungLong40xff.csv")
ax.set_title("Manufacturer B with data pattern 0xff at 40°C")

for retention in range(0,140, 20):
    data = df[df.retention==retention]
    ax.plot("tRCD", "FehlerProzent", '.', data=data, label=str(retention) + "s")

ax.set_yscale('symlog', linthreshy=0.01)
ax.set_ylim(ymin=0, ymax=100)
ax.set_xlim(xmin=0)
ax.legend(title="Retention Time:", fontsize= "10", ncol = 2)

ax.set_xlabel("tRCD in ns")
ax.set_ylabel("Errors in %")
#ax.yaxis.set_major_formatter(matplotlib.ticker.ScalarFormatter())
ax.yaxis.set_major_formatter(FormatStrFormatter('%.2f'))
ax.annotate
plt.show()
# %%
