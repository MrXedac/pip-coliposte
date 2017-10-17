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
#include <pip/api.h>

#define COLIPOSTE_SEND_INT	0x90

char *strcpy(char *d, const char *s){
	char *saved = d;
	while ((*d++ = *s++) != '\0');
	
	return saved;
}

void main()
{
    Pip_Debug_Puts("[SOURCE] Hello there! I'm about to write a nice letter to a friend of mine...\n");
	Pip_Debug_Puts("[SOURCE] Let's write a bit...\n");
	
	/* Mapped by the Coliposte parent partition */
	char* coliposte_buffer = (char*)0x800000;
	strcpy(coliposte_buffer, "Hello! This message comes from the Coliposte Source partition. Have a nice day ~");
	
	Pip_Debug_Puts("[SOURCE] Okay, now I'm sending it!\n");
	/* Now send the message! */
	while(1)
	{
			/* We'll send the message until it reaches! Good old spam method. */
			Pip_Notify(0, COLIPOSTE_SEND_INT, coliposte_buffer, 0);
	}

    for(;;);
}  
