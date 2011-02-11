require "net/imap"
require "test/unit"

class IMAPTest < Test::Unit::TestCase
  def test_parse_nomodesq
    parser = Net::IMAP::ResponseParser.new
    r = parser.parse(%Q'* OK [NOMODSEQ] Sorry, modsequences have not been enabled on this mailbox\r\n')
    assert_equal("OK", r.name)
    assert_equal("NOMODSEQ", r.data.code.name)
  end

  def test_encode_utf7
    utf8 = "\357\274\241\357\274\242\357\274\243"
    s = Net::IMAP.encode_utf7(utf8)
    assert_equal("&,yH,Iv8j-", s)

    utf8 = "\343\201\202&"
    s = Net::IMAP.encode_utf7(utf8)
    assert_equal("&MEI-&-", s)
  end

  def test_decode_utf7
    s = Net::IMAP.decode_utf7("&,yH,Iv8j-")
    utf8 = "\357\274\241\357\274\242\357\274\243"
    assert_equal(utf8, s)
  end
end
