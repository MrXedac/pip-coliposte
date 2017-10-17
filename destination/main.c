/*******************************************************************************/
/*  © Université Lille 1, The Pip Development Team (2015-2017)                 */
/*                                                                             */
/*  This software is a computer program whose purpose is to run a minimal,     */
/*  hypervisor relying on proven properties such as memory isolation.          */
/*                                                                             */
/*  This software is governed by the CeCILL license under French law and       */
/*  abiding by the rules of distribution of free software.  You can  use,      */
/*  modify and/ or redistribute the software under the terms of the CeCILL     */
/*  license as circulated by CEA, CNRS and INRIA at the following URL          */
/*  "http://www.cecill.info".                                                  */
/*                                                                             */
/*  As a counterpart to the access to the source code and  rights to copy,     */
/*  modify and redistribute granted by the license, users are provided only    */
/*  with a limited warranty  and the software's author,  the holder of the     */
/*  economic rights,  and the successive licensors  have only  limited         */
/*  liability.                                                                 */
/*                                                                             */
/*  In this respect, the user's attention is drawn to the risks associated     */
/*  with loading,  using,  modifying and/or developing or reproducing the      */
/*  software by the user in light of its specific status of free software,     */
/*  that may mean  that it is complicated to manipulate,  and  that  also      */
/*  therefore means  that it is reserved for developers  and  experienced      */
/*  professionals having in-depth computer knowledge. Users are therefore      */
/*  encouraged to load and test the software's suitability as regards their    */
/*  requirements in conditions enabling the security of their systems and/or   */
/*  data to be ensured and,  more generally, to use and operate it in the      */
/*  same conditions as regards security.                                       */
/*                                                                             */
/*  The fact that you are presently reading this means that you have had       */
/*  knowledge of the CeCILL license and that you accept its terms.             */
/*******************************************************************************/

#include <stdint.h>
#include <pip/fpinfo.h>
#include <pip/debug.h>
#include <pip/vidt.h>

#define COLIPOSTE_RECV_INT	0x91
#define COLIPOSTE_REG_INT	0x92
#define COLIPOSTE_ACK_INT	0x93

INTERRUPT_HANDLER(recvAsm, recvSignal)
	Pip_Debug_Puts("[DESTINATION] Oh, what a surprise, I received a letter! It should be at "); Pip_Debug_PutHex(data1); Pip_Debug_Puts(", let me check this...\n");
	Pip_Debug_Puts("[DESTINATION] It is written \"");
	Pip_Debug_Puts((char*)data1);
	Pip_Debug_Puts("\"\n");
	Pip_Debug_Puts("[DESTINATION] What a nice mail. I should send the ACK signal to Coliposte...\n");
	Pip_Notify(0, COLIPOSTE_ACK_INT, 0, 0);
END_OF_INTERRUPT

void main()
{
	Pip_RegisterInterrupt(COLIPOSTE_RECV_INT, &recvAsm, (uint32_t*)(0x804000 + 0x1000 - sizeof(uint32_t)));
	Pip_Notify(0, COLIPOSTE_REG_INT, 0, 0);
    Pip_Debug_Puts("[DESTINATION] Hello! I opened my mailbox.\n");
    for(;;);
}  
