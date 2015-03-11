--TEST--
suhosin input filter (suhosin.post.max_value_length - RFC1867 version)
--INI--
suhosin.log.syslog=0
suhosin.log.sapi=0
suhosin.log.script=0
suhosin.log.file=255
suhosin.log.file.time=0
suhosin.log.file.name={PWD}/suhosintest.$$.log.tmp
auto_append_file={PWD}/suhosintest.$$.log.tmp
suhosin.request.max_value_length=0
suhosin.post.max_value_length=3
--SKIPIF--
<?php include('../skipif.inc'); ?>
--COOKIE--
--GET--
--POST_RAW--
Content-Type: multipart/form-data; boundary=---------------------------20896060251896012921717172737
-----------------------------20896060251896012921717172737
Content-Disposition: form-data; name="var1"

1
-----------------------------20896060251896012921717172737
Content-Disposition: form-data; name="var2"

22
-----------------------------20896060251896012921717172737
Content-Disposition: form-data; name="var3"

333
-----------------------------20896060251896012921717172737
Content-Disposition: form-data; name="var4"

4444
-----------------------------20896060251896012921717172737
Content-Disposition: form-data; name="var5"

55 555
-----------------------------20896060251896012921717172737
Content-Disposition: form-data; name="var6"

666666
-----------------------------20896060251896012921717172737--
--FILE--
<?php
var_dump($_POST);
?>
--EXPECTF--
array(3) {
  ["var1"]=>
  string(1) "1"
  ["var2"]=>
  string(2) "22"
  ["var3"]=>
  string(3) "333"
}
ALERT - configured POST variable value length limit exceeded - dropped variable 'var4' (attacker 'REMOTE_ADDR not set', file '%s')
ALERT - configured POST variable value length limit exceeded - dropped variable 'var5' (attacker 'REMOTE_ADDR not set', file '%s')
ALERT - configured POST variable value length limit exceeded - dropped variable 'var6' (attacker 'REMOTE_ADDR not set', file '%s')
ALERT - dropped 3 request variables - (0 in GET, 3 in POST, 0 in COOKIE) (attacker 'REMOTE_ADDR not set', file '%s')

