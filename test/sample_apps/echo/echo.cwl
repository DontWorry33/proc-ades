cwlVersion: v1.0
$graph:
- class: CommandLineTool
  baseCommand: echo
  inputs:
    message:
      type: string
      inputBinding:
        position: 1
  outputs: []
