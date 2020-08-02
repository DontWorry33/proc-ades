﻿$graph:
- baseCommand: s3-slstr-tiling
  class: CommandLineTool
  hints:
    DockerRequirement:
      dockerPull: terradue/s3-slstr-preproc:1.0
  id: clt
  inputs:
    inp1:
      inputBinding:
        position: 1
        prefix: --input_reference
      type: Directory
    inp2:
      inputBinding:
        position: 2
        prefix: --tiling_level
    type: string
  outputs:
    results:
      outputBinding:
        glob: .
      type: Any
  requirements:
    EnvVarRequirement:
      envDef:
        PATH: /opt/anaconda/envs/env_s3/bin/:/opt/anaconda/envs/notebook/bin:/opt/anaconda/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
        PREFIX: /opt/anaconda/envs/env_s3
      ResourceRequirement: {}

- class: Workflow
  doc: This service takes as test_entites
  id: test_entites
  ows:version: 1.0 # workflow version
  inputs:

    int_parameter:
      label: Title of int_parameter
      doc: Abstract describing int_parameter
      type: integer
      default: 100

    boolean_parameter:
      label: Title of boolean_parameter
      doc: Abstract describing boolean_parameter
      type: boolean
      default: 100

    long_parameter:
      label: Title of long_parameter
      doc: Abstract describing long_parameter
      type: long
      default: 200

    float_parameter:
      label: Title of float_parameter
      doc: Abstract describing float_parameter
      type: float
      default: 23

    double_parameter:
      label: Title of double_parameter
      doc: Abstract describing double_parameter
      type: double
      default: 23

    string_parameter:
      label: Title of string_parameter
      doc: Abstract describing string_parameter
      type: string
      default: CIAO a te

    string_parameter_arr:
      label: Title of string_parameter_arr
      doc: Abstract describing string_parameter_arr
      type: string[]
      default: CIAO a te

    File_parameter:
      label: Title of File_parameter
      doc: Abstract describing File_parameter
      type: File

    Directory_parameter:
      label: Title of Directory_parameter
      doc: Abstract describing Directory_parameter
      type: Directory

    File_stac_parameter:
      label: Title of File_stac_parameter
      doc: Abstract describing File_stac_parameter
      type: File
      stac:catalog:
        stac:collection: input_file
        stac:href: catalog.json

    Directory_stac_parameter:
      label: Title of Directory_stac_parameter
      doc: Abstract describing Directory_stac_parameter
      type: Directory
      stac:catalog:
        stac:collection: input_file
        stac:href: catalog.json

    File_stac_parameter_arr:
      label: Title of File_stac_parameter_arr
      doc: Abstract describing File_stac_parameter_arr
      type: File[]
      stac:catalog:
        stac:collection: input_file
        stac:href: catalog.json

    Directory_stac_parameter_arr:
      label: Title of Directory_stac_parameter_arr
      doc: Abstract describing Directory_stac_parameter_arr
      type: Directory[]
      stac:catalog:
        stac:collection: input_file
        stac:href: catalog.json

    string_parameter_arr_items:
      label: Title of string_parameter_arr_items
      doc: Abstract describing string_parameter_arr_items
      type: 
        type: array
        items:
          - string

    string_with_options_parameter:
      label: Title of string_with_options_parameter
      type:
        type: enum
        symbols: [‘option1’, ‘options2’, ‘option3’]

    string_with_options_parameter_ser:
      label: Title of string_with_options_parameter_ser
      type: enum
      symbols: [‘optionA’, ‘optionsB’, ‘optionC’]

    string_parameter_arr_items_integer:
      label: Title of string_parameter_arr_items_integer
      doc: Abstract describing string_parameter_arr_items_integer
      type: 
        type: array
        items:
          - integer

    int_parameter_arr_spl:
      label: Title of int_parameter_arr_spl
      doc: Abstract describing int_parameter_arr_spl
      type: integer[]
      default: 100


    Directory_parameter_arr_spl:
      label: Title of Directory_parameter_arr_spl
      doc: Abstract describing Directory_parameter_arr_spl
      type: array
      items: Directory
      



  label: eoepca test_entites
  outputs:
  - id: wf_outputs
    outputSource:
    - node_1/results
    type:
      items: Directory
      type: array
  requirements:
  - class: ScatterFeatureRequirement
  steps:
    node_1:
      in:
        inp1: input_reference
        inp2: tiling_level
      out:
      - results
      run: '#clt'
      scatter: inp1
      scatterMethod: dotproduct
$namespaces:
  stac: http://www.me.net/stac/cwl/extension
cwlVersion: v1.0