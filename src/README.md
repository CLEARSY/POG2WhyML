# POG2WhyML

POG is an XML-based format for the representation of the proof obligations of the B method and of Event-B.
WhyML is the input language of the Why3 platform, a multi-prover platform for program verification.
POG2WhyML is a program that translates POG files to the WhyML language.

## Rationale of the translation

The structure of POG files is:

- a sequence of list of hypotheses, each such list being nested in `Define` XML elements.

- a sequence of list of proof obligations, each such list being   
  nested in a `Proof_Obligation` XML elements.
  
  The structure of `Proof_Obligation` XML elements is:

  1. A list of references to `Define` elements. 

  2. A list of hypothesis, nested in `Hypothesis` XML elements.

  3. A list of hypothesis, nested in `Local_Hyp` XML elements.

  4. A list of goals to prove, nested in `Simple_Goal` XML elements.

     The structure of a `Simple_Goal` element is:

     1. A list of references to `Local_Hyp`, in `Ref_Hyp` XML elements.

     1. The goal, in a `Goal` XML element.

The semantics of this structure is that each predicate nested
in a `Goal` must be proved taking as hypothesis:

1. The hypothesis in the referenced `Define` elements of its
  `Proof_Obligation` ancestor.

2. The hypothesis in the `Hypothesis` elements of its
  `Proof_Obligation` ancestor.

3. The hypothesis in the `Local_Hyp` referenced in the siblings 
  `Ref_Hyp` elements.

## Acknowledgments

The initial development of POG2WhyML has been funded by ANR 
(*Agence Nationale de la Recherche*), as part of the 
[BWare project](https://anr.fr/Project-ANR-12-INSE-0010)
number ANR-12-INSE-0010.
