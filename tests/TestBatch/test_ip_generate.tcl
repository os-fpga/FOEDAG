create_design ip_test
configure_ip MOCK_IP -mod_name mockup_ip -version 1.0 -PWidth=10 -out_file rs_ips/mockup_ip.v
ipgenerate
add_design_file rs_ips/mockup_ip.v
synth
