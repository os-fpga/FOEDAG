create_design ip_test
add_litex_ip_catalog ./
configure_ip MOCK_IP -mod_name mockup_ip -version 1.0 -PWidth=10 -out_file rs_ips/mockup_ip.v
ipgenerate
set_top_module use_ip
add_design_file rs_ips/mockup_ip.v use_ip.v
synth
