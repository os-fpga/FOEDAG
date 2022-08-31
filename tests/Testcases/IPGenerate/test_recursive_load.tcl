create_design ip_test
architecture ../../Arch/k6_frac_N10_tileable_40nm.xml ../../Arch/k6_N10_40nm_openfpga.xml
add_litex_ip_catalog ./IP_Catalog
foreach ip [ip_catalog] {
    puts "Found IP: $ip"
}