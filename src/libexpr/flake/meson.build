embeddded_files = [
  'call-flake.nix',
]

foreach file : embeddded_files
  libexpr_src += custom_target(
    (file + '.gen.hh'),
    output : (file + '.gen.hh'),
    input : file,
    command : [bash, '-c', gen_header, 'sh', '@OUTPUT@'],
  )
endforeach
