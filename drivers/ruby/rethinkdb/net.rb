require 'socket'
require 'thread'
require 'json'

module RethinkDB
  class Connection
    def self.last; @@last; end
    def debug_socket; @socket; end

    def start_listener
      @listener = Thread.new do
        loop do
          response_length = @socket.recv(4).unpack('L<')[0]
          response = @socket.recv(response_length)
          #TODO: Recovery
          begin
            protob = Response.new.parse_from_string(response)
          rescue
            p response
            abort("Bad Protobuf.")
          end
          @mutex.synchronize do
            @data[protob.token] = protob
            if (@waiters[protob.token])
              cond = @waiters.delete protob.token
              cond.signal
            end
          end
        end
      end
    end

    def dispatch msg
      # if msg.class == RethinkDB::RQL_Query
      #   File.open("sexp.txt", "a") {|f| f.write(msg.sexp.inspect+"\n")}
      # end
      if msg.class != Query then return dispatch msg.query end
      payload = msg.serialize_to_string
      packet = [payload.length].pack('L<') + payload
      @socket.send(packet, 0)
      return msg.token
    end

    def wait token
      @mutex.synchronize do
        (@waiters[token] = ConditionVariable.new).wait(@mutex) if not @data[token]
        return @data.delete token
      end
    end

    def continue token
      msg = Query.new
      msg.type = Query::QueryType::CONTINUE
      msg.token = token
      dispatch msg
    end

    def error(protob,err=RuntimeError)
      raise err,"RQL: #{protob.error_message}"
    end

    def token_iter(token)
      done = false
      data = []
      loop do
        if (data == [])
          break if done
          protob = wait token
          case protob.status_code
          when Response::StatusCode::SUCCESS_JSON then
            yield JSON.parse('['+protob.response[0]+']')[0]
            return false
          when Response::StatusCode::SUCCESS_PARTIAL then
            continue token
            data.push *protob.response
          when Response::StatusCode::SUCCESS_STREAM then
            data.push *protob.response
            done = true
          when Response::StatusCode::SUCCESS_EMPTY then
            return false
          when Response::StatusCode::BAD_QUERY then error protob,SyntaxError
          when Response::StatusCode::RUNTIME_ERROR then error protob,RuntimeError
          else error protob
          end
        end
        #yield JSON.parse("["+data.shift+"]")[0] if data != []
        yield JSON.parse('['+data.shift+']')[0] if data != []
        #yield data.shift if data != []
      end
      return true
    end
  end
end
