pme - Proof Minimize and Explain
================

[![Build Status](https://travis-ci.org/ryanberryhill/pme.svg?branch=master)](https://travis-ci.org/ryanberryhill/pme)

`pme` is a library and utility to minimize and explain proofs of safety (i.e.,
safe inductive invariants) for hardware circuits. It supports two main
functions: finding minimal safe inductive subsets (MSISes) of safe inductive
invariants and finding minimal inductive validity cores (MIVCs) of safe
circuits. Circuit inputs are assumed to follow
[HWMCC](http://fmv.jku.at/hwmcc17/) standards, given as an And-Inverter Graph (AIG) in
[aiger](http://fmv.jku.at/aiger/) format with a single output representing the
safety property. Proof inputs are in a simple textual format representing a CNF
formula over the latches in the given AIG. Each line is a space-delimited list
of numbers representing a clause, where each number is a literal from the AIG
that should be either a latch or its negation.

Currently, the tool supports minimizing invariants using at least the following
approaches:

1. A simple approach based on iteratively adding clauses until an inductive
formula is found that computes a non-minimal SIS (--simplemin)
2. Brute Force yielding a single MSIS (--bfmin)
3. An approach based on iterated steps of over- and under-approximation
of a particular MSIS based on the work of
[Ivrii et al.](https://repositories.lib.utexas.edu/bitstream/handle/2152/26151/FMCAD_2014.pdf?sequence=2#page=127) (--sisi)
4. A MARCO-based approach described in the [SAT'18 paper](http://www.eecg.utoronto.ca/~veneris/18sat.pdf)
(--marco)
5. A CAMUS-based approach described in the same SAT'18 paper (--camsis)

It also supports computing MIVCs using at least the following approaches:

1. A brute force approach based on IVC\_BF from Ghassabani et al. in "Efficient
   Generation of Inductive Validity Cores for Safety Properties" (--ivcbf)
2. An approach based on IVC\_UCBF from the same paper (--ivcucbf)
3. An approach that can operate like CAMUS or MARCO (--uivc)

Building
---------

To build pme you will need a C++11 compiler. The unit tests also depend on the
boost unit test framework.

```bash
    $ autoreconf -fi # Only if you checked this out from git
    $ ./configure
    $ make
    $ make check     # optional, requires boost
    $ make install   # optional, not recommended
```

Finding MSISes
---------------

All of the examples assume you are in the examples/proofs directory

```bash
    cd examples/proofs
```

To find MSISes requires specifying an algorithm, AIG, and proof. For instance,
to run MARCO-DOWN:

```bash
    pme --marco vis4arbitp1.aig vis4arbitp1.quip.pme
```

However, this won't produce any output unless something goes wrong. The -v
argument increases verbosity and can be specified multiple times. To use
verbosity 4 (the maximum):

```bash
    pme -vvvv --marco vis4arbitp1.aig vis4arbitp1.quip.pme
```

The --stats argument adds output of various statistics.

```bash
    pme --stats -vvvv --marco vis4arbitp1.aig vis4arbitp1.quip.pme
```

The --opt argument allows passing low-level internal arguments. For instance,
to run MARCO-UP requires the following options:

```bash
    pme -vvvv --marco \
        --opt marco_direction_down=false \
        --opt marco_direction_up=true \
        vis4arbitp1.aig vis4arbitp1.quip.pme
```

CAMSIS can be executed as follows.

```bash
    pme -vvvv --camsis --opt camsis_ar=false vis4arbitp1.aig vis4arbitp1.quip.pme
```

And confusingly, the FORQES-based approach is executed by just specifying
--camsis.

```bash
    pme -vvvv --camsis vis4arbitp1.aig vis4arbitp1.quip.pme
```


Finding MIVCs
---------------

All of the examples assume you are in the ivc-examples directory.

```bash
    cd ivc-examples
```

Finding MIVCs is similar to finding MSISes, but there is no need to specify a
proof. For instance, MARCO-DOWN is executed as follows.

```bash
    pme -vvvv --uivc visemodel.aig
```

Most options are controlled through the --opt parameter. To execute CAMUS:

```bash
    pme -vvvv --uivc --opt uivc_direction_down=false --opt uivc_direction_up=true --opt uivc_upfront_nmax=inf visemodel.aig
```

To execute MARCO-UP:

```bash
    pme -vvvv --uivc --opt uivc_direction_down=false --opt uivc_rection_up=true visemodel.aig
```

Or with an important optimization:

```bash
    pme -vvvv --uivc --opt uivc_direction_down=false --opt uivc_rection_up=true --opt uivc_mcs_grow=true visemodel.aig 
```

Finally, a MARCO-DOWN configuration with good performance:
```bash
    pme -vvvv --uivc --opt uivc_proof_cache=1 --opt uivc_cex_cache=8 --opt uivc_shrink_cached_proofs=true --opt uivc_clever_issafe=true --opt uivc_coi_hints=true --opt uivc_upfront_nmax=2 visemodel.aig
```
