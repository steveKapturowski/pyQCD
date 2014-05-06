"""
Here we compute two-point functions for all 256 combinations of meson
interpolators using different smearing combinations at the source and sink.

To do this we write a custom measurement function to reuse the propagators when
performing smearing at the sink.

We load the ensemble generated by the generate_configs.py script here to save
computation time.
"""

import numpy as np

import pyQCD

def smear_propagator(lattice, prop, n_smears, smearing_param):
    """Smear the supplied propagator at the sink, and set the smearing
    parameters in the propagator to those characterising the smearing"""

    new_prop = np.zeros(prop.shape, dtype=np.complex)
    
    for alpha in xrange(4):
        for a in xrange(3):
            prop_element = prop[:, :, :, :, :, alpha, :, a]
            
            new_prop[:, :, :, :, :, alpha, :, a] \
              = lattice.apply_jacobi_smearing(prop_element, n_smears,
                                              smearing_param)
            
    return new_prop

def compute_correlators(lattice, mass_1, mass_2):
    """Computes all 256 meson correlation functions for the two supplied
    quark masses and all four possible source/sink smearing combinations"""

    out = {}
    
    # Next we need the propagators. We do unsmeared first (the default).
    prop_LL_mass_1 = lattice.get_wilson_propagator(mass_1, verbosity=2,
                                                   precondition=True)
    prop_LL_mass_2 = lattice.get_wilson_propagator(mass_2, verbosity=2,
                                                   precondition=True)
    
    # Smear these propagators with two jacobi smears and a parameter of 0.4
    prop_LS_mass_1 = smear_propagator(lattice, prop_LL_mass_1, 2, 0.4)
    prop_LS_mass_2 = smear_propagator(lattice, prop_LL_mass_2, 2, 0.4)
    
    # Now we need compute the propagators with source smearing.
    # We do unsmeared first (the default).
    prop_SL_mass_1 = lattice.get_wilson_propagator(mass_1,
                                                   num_source_smears=2,
                                                   source_smearing_param=0.4,
                                                   verbosity=2,
                                                   precondition=True)
    prop_SL_mass_2 = lattice.get_wilson_propagator(mass_2,
                                                   num_source_smears=2,
                                                   source_smearing_param=0.4,
                                                   verbosity=2,
                                                   precondition=True)
    
    # Smear these propagators with two jacobi smears and a parameter of 0.4
    prop_SS_mass_1 = smear_propagator(lattice, prop_SL_mass_1, 2, 0.4)
    prop_SS_mass_2 = smear_propagator(lattice, prop_SL_mass_2, 2, 0.4)
    
    mass_1_props = [prop_LL_mass_1,
                    prop_LS_mass_1,
                    prop_SL_mass_1,
                    prop_SS_mass_1]
    
    mass_2_props = [prop_LL_mass_2,
                    prop_LS_mass_2,
                    prop_SL_mass_2,
                    prop_SS_mass_2]
    
    prop_pairings = [(p1, p2) for p1 in mass_1_props for p2 in mass_2_props]

    smearing_combinations \
      = ["{}{}".format(x, y)
         for x in ["LL", "LS", "SL", "SS"]
         for y in ["LL", "LS", "SL", "SS"]]
        
    # Now we go through and contract all propagator combinations    
    for (prop1, prop2), smear_comb in zip(prop_pairings, smearing_combinations):

        correlators = pyQCD.compute_meson_corr256(prop1, prop2)
        # Merge the smearing labels into existing dict keys
        correlators = dict([((key,) + (smear_comb,), value)
                            for key, value in correlators.iteritems()])
        out.update(correlators)
        
    return out

if __name__ == "__main__":
    
    # As in generate_configs.py, we create a simulation. Since we're providing
    # an ensemble the latter two parameters do nothing here.
    simulation = pyQCD.Simulation(100, 10, 100)
    
    # Create the lattice. Again, the action and beta value do nothing here.
    simulation.create_lattice(4, 8, "rectangle_improved", 5.5)

    # Here we load the ensemble
    simulation.load_ensemble("4c8_ensemble.zip")
    
    # Now add our new measurement. compute_correlators returns a TwoPoint
    # object, so we specify that here. Here we need to provide parameters, so
    # they're specified as keyword args.
    simulation.add_measurement(compute_correlators,
                               "4c8_correlators_m0.4_m0.03.zip",
                               kwargs={"mass_1": 0.4, "mass_2": 0.03},
                               meas_message="Computing correlators")

    # And run the simulation
    simulation.run()
