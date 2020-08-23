import csv
import numpy
from matplotlib import pyplot as plt

def getPlotVectors(throughputs):
	mins = []
	maxs = []
	avgs = []
	xs = numpy.linspace(5,30,6)

	for i in range(1,7):
		candidates = [float(throughputs[0][i]), float(throughputs[1][i]), float(throughputs[2][i])]
		
		minVal = min(candidates)
		maxVal = max(candidates)
		meanVal = numpy.mean(candidates)
		
		mins.append(minVal)
		maxs.append(maxVal)
		avgs.append(meanVal)

	ys = numpy.array(avgs)
	lowDys = ys - numpy.array(mins)
	highDys = numpy.array(maxs) - ys
	return xs, ys, lowDys, highDys

resultsCsv = csv.reader(open("results.csv", 'r'))
resultsTable = [line for line in resultsCsv]

plainThroughputs = resultsTable[4:7]
plainXs, plainYs, plainLowDys, plainHighDys = getPlotVectors(plainThroughputs)

mod1Throughputs = resultsTable[16:19]
mod1Xs, mod1Ys, mod1LowDys, mod1HighDys = getPlotVectors(mod1Throughputs)

mod2Throughputs = resultsTable[28:31]
mod2Xs, mod2Ys, mod2LowDys, mod2HighDys = getPlotVectors(mod2Throughputs)

mod3Throughputs = resultsTable[40:43]
mod3Xs, mod3Ys, mod3LowDys, mod3HighDys = getPlotVectors(mod3Throughputs)

mod4Throughputs = resultsTable[52:55]
mod4Xs, mod4Ys, mod4LowDys, mod4HighDys = getPlotVectors(mod4Throughputs)

plt.figure()
plt.errorbar(plainXs, plainYs, yerr=[plainLowDys, plainHighDys], label="Plain OLSR")
plt.errorbar(mod1Xs, mod1Ys, yerr=[mod1LowDys, mod1HighDys], label="Modification 1")
plt.errorbar(mod2Xs, mod2Ys, yerr=[mod2LowDys, mod2HighDys], label="Modification 2")
plt.errorbar(mod3Xs, mod3Ys, yerr=[mod3LowDys, mod3HighDys], label="Modification 3")
plt.errorbar(mod4Xs, mod4Ys, yerr=[mod4LowDys, mod4HighDys], label="Modification 4")
plt.legend(loc=0)
plt.title("Throughput vs flows (20m/s)")
plt.xlabel("Number of CBR data flows.")
plt.ylabel("Total network throughput (kbps).")
plt.xlim(0,31)
plt.ylim(0)
plt.show()
