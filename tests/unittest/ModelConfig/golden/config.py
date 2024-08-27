import re
def get_pin_info(name) :
  bank = 0
  is_clock = False
  index = 0
  pair_index = 0
  ab_io = 0
  ab_name = ''
  root_bank_mux_location = ''
  root_bank_mux_resource = ''
  root_bank_mux_core_input_index = 0
  root_mux_input_index = 0
  if name.find('BOOT_CLOCK#') == 0:
    type = 'BOOT_CLOCK'
    index = int(name[11:])
  else :
    m = re.search(r'H(P|R?)_(\d?)(|_CC?)_(\d+?)_(\d\d?)(P|N?)', name)
    assert m != None
    assert len(m.groups()) == 6
    type = 'HP' if m.group(1) == 'P' else ('HVL' if m.group(2) in ['1', '2'] else 'HVR')
    bank = 0 if m.group(2) in ['1', '3'] else 1
    is_clock = m.group(2) == '_CC'
    index = int(m.group(4))
    pair_index = int(m.group(5))
    ab_io = 0 if (pair_index < 10) else 1
    ab_name = '%c' % (ord('A') + ab_io)
    root_name = 'u_GBOX_HP_40X2' if type == 'HP' else ('u_GBOX_HV_40X2_VL' if type == 'HVL' else 'u_GBOX_HV_40X2_VR')
    root_bank_mux_location = '%s.u_gbox_root_bank_clkmux_%d' % (root_name, bank)
    root_bank_mux_resource = '%s (Bank %s)' % (root_bank_mux_location, ab_name)
    root_bank_mux_core_input_index = index - (20 * ab_io)
    root_mux_input_index = 0 if type == 'HP' else (8 if type == 'HVL' else 16)
    root_mux_input_index += ((2 * bank) + ab_io)
  return [type, bank, is_clock, index, pair_index, ab_io, ab_name, root_bank_mux_location, root_bank_mux_resource, root_bank_mux_core_input_index, root_mux_input_index]
def fclk_use_pll_resource(fclk) :
  pll_resource = 0
  if fclk.find('hvl_fclk_') == 0 :
    pll_resource = 0
  elif fclk.find('hvr_fclk_') == 0 :
    pll_resource = 1
  elif fclk.find('hp_fclk_0') == 0 :
    pll_resource = 0
  elif fclk.find('hp_fclk_1') == 0 :
    pll_resource = 1
  else :
    raise Exception('Unknown FCLK %s' % fclk)
  return [pll_resource]
