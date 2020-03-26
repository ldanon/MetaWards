# MetaWards

## References

- _"Individual identity and movement networks for disease metapopulations"_
Matt J. Keeling, Leon Danon, Matthew C. Vernon, Thomas A. House
Proceedings of the National Academy of Sciences May 2010, 107 (19) 8866-8870; DOI: [10.1073/pnas.1000416107](https://doi.org/10.1073/pnas.1000416107)

- _"A spatial model of CoVID-19 transmission in England and Wales: early spread and peak timing"_
Leon Danon, Ellen Brooks-Pollock, Mick Bailey, Matt J Keeling
medRxiv 2020.02.12.20022566; doi: [10.1101/2020.02.12.20022566](https://doi.org/10.1101/2020.02.12.20022566)

## Summary

This repository has the code used for the papers and preprints in the References section

## Dependancies

The code depends on the [GSL - the GNU Scientific Library](https://www.gnu.org/software/gsl/), install with your operating 
system's package manager or load the appropriate module if you are on an HPC system.

## Building

Clone the repository 

```ShellSession
[user@host ~]$ git clone https://github.com/ldanon/MetaWards.git
```

Change in to the repository and run `make`

```ShellSession
[user@host ~]$ cd MetaWards
[user@host MetaWards]$ make
```

This produces an executable named `wards.o`

## Running

MetaWards currently expects the data files to be found in a hard coded path under your home Directory 
`~/GitHub/MetaWards/2011Data/`. If you don't have the cloned repository there you can create a symlink.

### Running a single experiment

```ShellSession
[user@host MetaWards]$ mkdir expt
[user@host MetaWards]$ cd expt
[user@host MetaWards]$ ../wards.o 42 Testing/ncovparams.csv  4 1
```

Where the command line arguments are

```
./wards.o <RANDOM_SEED> <PARAMETER_FILE> <SOME_NUMBER> <SOME_OTHER_NUMBER>
```

### Running an ensemble

To run multiple experiments use the driving shellscript `run_repeats.sh`

