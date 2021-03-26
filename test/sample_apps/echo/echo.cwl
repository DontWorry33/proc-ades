cwlVersion: v1.0
- class: CommandLineTool
  baseCommand: echo
  inputs:
    message:
      type: string
      inputBinding:
        position: 1
  outputs: []
- class: Workflow
  id: echo_test
  label: Echo Test
  doc: Echo Test
  inputs:
    message:
      type: string
      doc: "Something to echo..."
      label: Echo string
