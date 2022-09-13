module generic (output O, input I0, input I1, input S);
	assign O = (S)? I1 : I0;
endmodule

