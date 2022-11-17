# PolarCodesDNN
See related paper (ArXiv link soon available) for more details.

Using Deep Neural Networks to Predict and Improve Polar Codes Error Rates.

## Generating datasets

```bash
git clone --recursive git@github.com:bonben/gad.git
mkdir gad/build
cd gad/build
cmake .. -G"Ninja" -DCMAKE_CXX_COMPILER="g++" -DCMAKE_BUILD_TYPE="Release" -DCMAKE_CXX_FLAGS="-funroll-loops -march=native"
ninja
```

The following command line allows for example to generate the datasets of (256,128) codes.

 ```bash
 ./bin/gen_aff3ct_datasets --gad-seed "2" -s "3.2" -e "1000" --enc-fb-gen-method "GA" -K "128" -N "256" --src-type "RAND" --src-implem "FAST" --chn-type "AWGN" --chn-implem "FAST" --dec-type "SCL" --dec-simd "INTRA"  --dec-polar-nodes "{R0,R0L,R1,REP,REPL,SPC_4}" -L "4" -R "18" -S "18000" --gad-term
 ```

Refer to the CLI help (`-h` argument) for further options.

## Datasets
Available in the [nn_experiments](nn_experiments) folder as `txt` files, should be straightforward to use.

## NN training and experiments
Simply run the notebook file in [nn_experiments](nn_experiments) to reproduce results from the paper. Activate the option "run_simulations" to reproduce hyperparameter figures.
