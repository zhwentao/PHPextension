<?php 
$i = 10000;
while($i--) {
    echo replace_word("[{$i}]oppo 百度，阿里巴巴，滴滴小米，头条baidu{$i}---"). "\r\n";
}
/*
	you can add regression tests for your extension here

  the output of your test code has to be equal to the
  text in the --EXPECT-- section below for the tests
  to pass, differences between the output and the
  expected text are interpreted as failure

	see php5/README.TESTING for further information on
  writing regression tests
*/
?>
