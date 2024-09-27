import re
MAX_BOOT_CLOCK_RESOURCE = 1
hp_banks = ['HP_%d' % i for i in [1, 2]]
hr_banks = ['HR_%d' % i for i in [1, 2, 3, 5]]
all_banks = hp_banks + hr_banks
bank_pin_count = 40
CC_index = [18, 19, 38, 39]
exclude_index = []
pin_list = ['%s%d_%d%c' % ('CC_' if i in CC_index else '', i, i//2, 'N' if i%2 else 'P') for i in range(bank_pin_count) if i not in exclude_index]
cc_p_pin_list = [pin for pin in pin_list if (pin.find('CC_') == 0 and pin[-1] == 'P')]
g_all_pins = ['%s_%s' % (i, j) for i in all_banks for j in pin_list]
g_all_clock_pins = ['%s_%s' % (i, j) for i in all_banks for j in cc_p_pin_list]
g_all_pll_clock_pins = [pin for pin in g_all_clock_pins]
g_boot_clock_resources = 0
g_pin_resources = {}
g_input_gearbox_width = {}
g_output_gearbox_width = {}
def parse_pin_location(location):
  assert location in g_all_pins
  m = re.search(r'H(P|R?)_(\d?)(|_CC?)_(\d+?)_(\d\d?)(P|N?)', location)
  assert m != None
  assert len(m.groups()) == 6
  type = 'HP' if m.group(1) == 'P' else ('HVL' if m.group(2) in ['1', '2'] else 'HVR')
  bank = 0 if m.group(2) in ['1', '3'] else 1
  is_clock = m.group(3) == '_CC'
  index = int(m.group(4))
  pair_index = int(m.group(5))
  assert pair_index == (index//2)
  ab_io = 0 if (pair_index < 10) else 1
  ab_name = '%c' % (ord('A') + ab_io)
  return [m, type, bank, is_clock, index, pair_index, ab_io, ab_name]
def get_peer_location(location):
  (m, type, bank, is_clock, index, pair_index, ab_io, ab_name) = parse_pin_location(location)
  pn = 'P' if m.group(6) == 'N' else 'N'
  index = int(m.group(4)) & ~1
  index += (1 if pn == 'N' else 0)
  peer_location = 'H%s_%s%s_%d_%s%s' % (m.group(1), m.group(2), m.group(3), index, m.group(5), pn)
  return [m.group(6), peer_location]
def validate_data_width_parameter(location, width, gearboxes):
  (self_pn, peer_location) = get_peer_location(location)
  result = width >= 3 and width <= (10 if self_pn == 'P' else 5)
  result = result and ((peer_location not in gearboxes) or (gearboxes[peer_location] <= 5 and width <=5))
  gearboxes[location if result else ''] = width
  gearboxes.pop('', None)
  return result
def get_pin_info(name):
  bank = 0
  is_clock = False
  index = 0
  pair_index = 0
  ab_io = 0
  ab_name = ''
  if name.find('BOOT_CLOCK#') == 0:
    type = 'BOOT_CLOCK'
    index = int(name[11:])
    model_name = 'hp_40x2.rc_osc_50mhz'
  elif name.find('FABRIC_CLKBUF#') == 0:
    type = 'FABRIC_CLKBUF'
    index = int(name[14:])
    model_name = 'fclk_buf'
  else :
    (m, type, bank, is_clock, index, pair_index, ab_io, ab_name) = parse_pin_location(name)
    model_name = '%s_40x2.bank%d_hpio.gearbox_%s[%d]' % (type.lower(), bank, m.group(6), pair_index)
  return [type, bank, is_clock, index, pair_index, ab_io, ab_name, model_name]
