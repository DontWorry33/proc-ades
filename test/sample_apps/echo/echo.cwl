cwlVersion: v1.0

$namespaces:
  ows: http://www.opengis.net/ows/1.1
  schema: https://json-schema.org/draft/2019-09/schema


$graph:
- class: CommandLineTool
  baseCommand: echo
  id: echo_test
  inputs:
    message:
      type: string
      inputBinding:
        position: 1
  outputs:
    echo_out:
      type: stdout
- class: Workflow
  id: echo_test_generation 
  label: Echo Test
  doc: Echo Test
  ows:version: 1.0 # workflow version
  inputs:
    message:
      doc: string to echo
      label: Echo tTest
      type: string
  outputs:
    echo_out:
      label: Echo output
      type: stdout
  steps:
    step1:
      in: 
        arg1: message
      out:
      - echo_out
      run:
        '#echo_test'
